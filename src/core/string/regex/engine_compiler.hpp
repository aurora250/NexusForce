#ifndef NEFORCE_CORE_STRING_REGEX_ENGINE_COMPILER_SRC__
#define NEFORCE_CORE_STRING_REGEX_ENGINE_COMPILER_SRC__
#include "engine_types.hpp"
NEFORCE_BEGIN_NAMESPACE__

class compiler {
public:
    compiler(const vector<node>& entries, const vector<char_class>& classes, const int capture_count) :
    nodes_(entries),
    classes_(classes),
    capture_entry_(static_cast<size_t>(capture_count) + 1, -1) {}

    void configure(const uint32_t options) noexcept {
        utf_ = (options & opt_utf) != 0;
        dollar_endonly_ = (options & static_cast<uint32_t>(opt_dollar_endonly)) != 0;
        multiline_circumflex_ = (options & opt_alt_circumflex) != 0;
    }

    void compile(const int32_t root) {
        code.reserve(48);
        start_valid_ = collect_first(root, start_bits_);

        prepare_follow(root);

        emit(op::save, 0);
        compile_node(root);
        emit(op::save, 1);
        emit(op::match);

        risk_state state;
        scan_risk(root, state);

        finish_required(root);
        detect_start_run(root);

        for (const auto& fixup: call_fixups_) {
            int32_t target = 0;

            if (fixup.group != 0) {
                if (static_cast<size_t>(fixup.group) >= capture_entry_.size() ||
                    capture_entry_[static_cast<size_t>(fixup.group)] < 0) {
                    throw compile_failure{err_unknown_group_reference, 0};
                }
                target = capture_entry_[static_cast<size_t>(fixup.group)];
            }

            code[static_cast<size_t>(fixup.ip)].a = target;
        }
    }

    vector<instruction> code;
    int loop_count = 0;

private:
    struct call_fixup {
        int32_t ip;
        int32_t group;
    };

    int32_t emit(const op kind, const int32_t a = 0, const int32_t b = 0, const int32_t c = 0, const int32_t d = 0,
                 const int32_t e = 0) {
        instruction entry;
        entry.code = kind;
        entry.a = a;
        entry.b = b;
        entry.c = c;
        entry.d = d;
        entry.e = e;
        code.push_back(entry);
        return static_cast<int32_t>(code.size() - 1);
    }

    void measure(const int32_t index, int64_t& minimum, int64_t& maximum) const {
        const auto slot = static_cast<size_t>(index);

        if (slot < measure_cache_.size() && measure_cache_[slot].state == 2) {
            minimum = measure_cache_[slot].minimum;
            maximum = measure_cache_[slot].maximum;
            return;
        }

        measure_raw(index, minimum, maximum);

        if (slot < measure_cache_.size()) {
            measure_cache_[slot].minimum = minimum;
            measure_cache_[slot].maximum = maximum;
            measure_cache_[slot].state = 2;
        }
    }

    void measure_raw(const int32_t index, int64_t& minimum, int64_t& maximum) const {
        const node& entry = nodes_[static_cast<size_t>(index)];
        minimum = 0;
        maximum = 0;

        switch (entry.type) {
            case node::kind::empty:
            case node::kind::begin_line:
            case node::kind::end_line:
            case node::kind::begin_subject:
            case node::kind::end_subject:
            case node::kind::end_subject_final:
            case node::kind::word_boundary:
            case node::kind::non_word_boundary:
            case node::kind::look:
            case node::kind::atomic:
            case node::kind::keep:
            case node::kind::begin_search:
                return;
            case node::kind::literal:
                minimum = 1;
                maximum = utf_ ? 4 : 1;
                return;
            case node::kind::any:
            case node::kind::char_class:
            case node::kind::cluster:
                minimum = 1;
                maximum = utf_ ? 32 : 1;
                return;
            case node::kind::backref:
            case node::kind::call:
                maximum = -1;
                return;
            case node::kind::concat:
                for (const int32_t child: entry.children) {
                    int64_t child_minimum = 0;
                    int64_t child_maximum = 0;
                    measure(child, child_minimum, child_maximum);
                    minimum += child_minimum;

                    if (maximum < 0 || child_maximum < 0) {
                        maximum = -1;
                    } else {
                        maximum += child_maximum;
                    }
                }
                return;
            case node::kind::alternate: {
                bool first = true;

                for (const int32_t child: entry.children) {
                    int64_t child_minimum = 0;
                    int64_t child_maximum = 0;
                    measure(child, child_minimum, child_maximum);

                    if (first || child_minimum < minimum) {
                        minimum = child_minimum;
                    }

                    if (first || maximum < 0 || child_maximum < 0) {
                        maximum = child_maximum;
                    } else if (child_maximum > maximum) {
                        maximum = child_maximum;
                    }

                    first = false;
                }
                return;
            }
            case node::kind::repeat: {
                int64_t child_minimum = 0;
                int64_t child_maximum = 0;
                measure(entry.children[0], child_minimum, child_maximum);
                minimum = child_minimum * entry.minimum;

                if (entry.maximum < 0 || child_maximum < 0) {
                    maximum = -1;
                } else {
                    maximum = child_maximum * entry.maximum;
                }
                return;
            }
            case node::kind::capture:
            case node::kind::group:
                measure(entry.children[0], minimum, maximum);
                return;
            case node::kind::conditional: {
                bool first = true;

                for (const int32_t child: entry.children) {
                    int64_t child_minimum = 0;
                    int64_t child_maximum = 0;
                    measure(child, child_minimum, child_maximum);

                    if (first || child_minimum < minimum) {
                        minimum = child_minimum;
                    }

                    if (first || maximum < 0 || child_maximum < 0) {
                        maximum = child_maximum;
                    } else if (child_maximum > maximum) {
                        maximum = child_maximum;
                    }

                    first = false;
                }

                if (entry.group < 0) {
                    minimum = 0;
                    maximum = 0;
                } else if (entry.children.size() == 1) {
                    minimum = 0;
                }

                return;
            }
        }
    }

    void compile_alternate(const node& entry) {
        vector<int32_t> jumps;

        for (size_t index = 0; index + 1 < entry.children.size(); ++index) {
            const auto split = emit(op::split);
            const auto branch = static_cast<int32_t>(code.size());
            compile_node(entry.children[index]);
            jumps.push_back(emit(op::jump));
            const auto following = static_cast<int32_t>(code.size());
            code[static_cast<size_t>(split)].a = branch;
            code[static_cast<size_t>(split)].b = following;
        }

        compile_node(entry.children.back());

        const auto end = static_cast<int32_t>(code.size());

        for (const int32_t jump: jumps) {
            code[static_cast<size_t>(jump)].a = end;
        }
    }

    void compile_loop_body(const node& entry, const bool lazy) {
        const auto child = entry.children[0];

        for (int32_t count = 0; count < entry.minimum; ++count) {
            compile_node(child);
        }

        if (entry.maximum < 0) {
            const auto slot = loop_count++;
            const auto split = emit(op::split);
            const auto body = static_cast<int32_t>(code.size());
            emit(op::loop_enter, slot);
            compile_node(child);
            const auto check = emit(op::loop_check, slot);
            emit(op::jump, split);
            const auto after = static_cast<int32_t>(code.size());

            if (lazy) {
                code[static_cast<size_t>(split)].a = after;
                code[static_cast<size_t>(split)].b = body;
            } else {
                code[static_cast<size_t>(split)].a = body;
                code[static_cast<size_t>(split)].b = after;
            }

            code[static_cast<size_t>(check)].b = after;
            return;
        }

        for (int32_t count = entry.minimum; count < entry.maximum; ++count) {
            const int32_t split = emit(op::split);
            const auto body = static_cast<int32_t>(code.size());
            compile_node(child);
            const auto after = static_cast<int32_t>(code.size());

            if (lazy) {
                code[static_cast<size_t>(split)].a = after;
                code[static_cast<size_t>(split)].b = body;
            } else {
                code[static_cast<size_t>(split)].a = body;
                code[static_cast<size_t>(split)].b = after;
            }
        }
    }

    void compile_wrapped(const op kind, const int32_t index, const int32_t look_kind) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        int64_t minimum = 0;
        int64_t maximum = 0;

        if (kind == op::look && (look_kind == 3 || look_kind == 4)) {
            measure(entry.children[0], minimum, maximum);

            if (maximum < 0) {
                throw compile_failure{err_lookbehind_not_fixed, entry.offset};
            }

            maximum = min<int64_t>(maximum, 0x7FFF);
        }

        const int32_t wrapper = emit(kind, 0, look_kind, static_cast<int32_t>(minimum), static_cast<int32_t>(maximum));
        emit(op::jump);
        const auto body = static_cast<int32_t>(code.size());
        compile_node(entry.children[0]);
        emit(op::accept);
        const auto after = static_cast<int32_t>(code.size());

        code[static_cast<size_t>(wrapper)].a = body;
        code[static_cast<size_t>(wrapper)].e = after;
        code[static_cast<size_t>(wrapper) + 1UL].a = after;
    }

    void compile_node(const int32_t index) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        switch (entry.type) {
            case node::kind::empty:
                break;
            case node::kind::literal:
                emit(op::char_match, static_cast<int32_t>(entry.code_point), entry.caseless ? 1 : 0);
                break;
            case node::kind::any:
                emit(op::any, 0, entry.dotall ? 1 : 0);
                break;
            case node::kind::char_class:
                emit(op::char_class, entry.class_index);
                break;
            case node::kind::concat:
                for (const int32_t child: entry.children) {
                    compile_node(child);
                }
                break;
            case node::kind::alternate:
                compile_alternate(entry);
                break;
            case node::kind::repeat:
                if (entry.maximum == 0 && entry.minimum == 0) {
                    break;
                }

                if (entry.minimum == entry.maximum && entry.minimum >= 1 && entry.minimum <= 16 &&
                    emit_fixed_run(entry)) {
                    break;
                }

                if (entry.mode == node::repeat_mode::possessive ||
                    (entry.mode == node::repeat_mode::greedy && can_possessify(index))) {
                    final_possessive_[static_cast<size_t>(index)] = 1;

                    if (entry.maximum < 0 && entry.minimum <= 1 && emit_single_char_run(entry)) {
                        break;
                    }

                    const auto wrapper = emit(op::atomic);
                    emit(op::jump);
                    const auto body = static_cast<int32_t>(code.size());
                    compile_loop_body(entry, false);
                    emit(op::accept);
                    const auto after = static_cast<int32_t>(code.size());
                    code[static_cast<size_t>(wrapper)].a = body;
                    code[static_cast<size_t>(wrapper)].e = after;
                    code[static_cast<size_t>(wrapper) + 1UL].a = after;
                    break;
                }

                if (entry.mode == node::repeat_mode::greedy && entry.maximum < 0 && emit_greedy_char_loop(entry)) {
                    break;
                }

                compile_loop_body(entry, entry.mode == node::repeat_mode::lazy);
                break;
            case node::kind::capture: {
                const int32_t slot = entry.capture * 2;
                emit(op::save, slot);

                if (static_cast<size_t>(entry.capture) < capture_entry_.size()) {
                    capture_entry_[static_cast<size_t>(entry.capture)] = static_cast<int32_t>(code.size());
                }

                compile_node(entry.children[0]);
                emit(op::save, slot + 1);
                break;
            }
            case node::kind::group:
                compile_node(entry.children[0]);
                break;
            case node::kind::look:
                compile_wrapped(op::look, index, entry.look_kind);
                break;
            case node::kind::atomic:
                compile_wrapped(op::atomic, index, 0);
                break;
            case node::kind::backref:
                emit(op::backref, entry.group, entry.caseless ? 1 : 0);
                break;
            case node::kind::begin_line:
                emit(op::begin_line, entry.multiline ? 1 : 0, multiline_circumflex_ ? 1 : 0);
                break;
            case node::kind::end_line:
                emit(op::end_line, entry.multiline ? 1 : 0, 0, dollar_endonly_ ? 1 : 0);
                break;
            case node::kind::begin_subject:
                emit(op::begin_subject);
                break;
            case node::kind::end_subject:
                emit(op::end_subject);
                break;
            case node::kind::end_subject_final:
                emit(op::end_subject_final);
                break;
            case node::kind::word_boundary:
                emit(op::word_boundary, entry.ucp ? 1 : 0);
                break;
            case node::kind::non_word_boundary:
                emit(op::non_word_boundary, entry.ucp ? 1 : 0);
                break;
            case node::kind::cluster:
                emit(op::cluster);
                break;
            case node::kind::keep:
                emit(op::keep);
                break;
            case node::kind::begin_search:
                emit(op::begin_search);
                break;
            case node::kind::conditional: {
                if (entry.group < 0) {
                    const auto jump = emit(op::jump);
                    compile_node(entry.children[0]);
                    emit(op::match);
                    code[static_cast<size_t>(jump)].a = static_cast<int32_t>(code.size());
                    break;
                }

                const auto branch = emit(op::branch_group, entry.group);
                emit(op::jump);
                const auto yes_start = static_cast<int32_t>(code.size());
                compile_node(entry.children[0]);
                const auto yes_jump = emit(op::jump);
                const auto no_start = static_cast<int32_t>(code.size());

                if (entry.children.size() > 1) {
                    compile_node(entry.children[1]);
                }

                const auto after = static_cast<int32_t>(code.size());
                code[static_cast<size_t>(branch)].b = yes_start;
                code[static_cast<size_t>(branch)].c = no_start;
                code[static_cast<size_t>(branch) + 1UL].a = after;
                code[static_cast<size_t>(yes_jump)].a = after;
                break;
            }
            case node::kind::call: {
                const int32_t slot = emit(op::call);
                call_fixups_.push_back(call_fixup{slot, entry.group});
                break;
            }
        }
    }

public:
    struct byte_bits {
        uint8_t bits[32] = {};

        void set(const uint32_t byte) noexcept { bits[byte >> 3] |= static_cast<uint8_t>(1U << (byte & 7U)); }

        NEFORCE_NODISCARD bool test(const uint32_t byte) const noexcept {
            return (bits[byte >> 3] & static_cast<uint8_t>(1U << (byte & 7U))) != 0;
        }

        void merge(const byte_bits& other) noexcept {
            for (size_t index = 0; index < sizeof(bits); ++index) {
                bits[index] = static_cast<uint8_t>(bits[index] | other.bits[index]);
            }
        }

        NEFORCE_NODISCARD bool intersects(const byte_bits& other) const noexcept {
            for (size_t index = 0; index < sizeof(bits); ++index) {
                if ((bits[index] & other.bits[index]) != 0) {
                    return true;
                }
            }

            return false;
        }
    };

    byte_bits start_bits_;
    bool start_valid_ = false;
    bool start_anchored_ = false;
    bool nested_quantifier_ = false;
    bool exponential_risk_ = false;
    int required_count_ = 0;
    int start_run_class_ = -1;
    int start_run_literal_ = -1;
    uint8_t required_masks_[2][32] = {};

private:
    struct byte_mask {
        uint8_t bits[32] = {};

        void set(const uint32_t byte) noexcept { bits[byte >> 3] |= static_cast<uint8_t>(1U << (byte & 7U)); }

        NEFORCE_NODISCARD int popcount() const noexcept {
            int total = 0;

            for (size_t index = 0; index < sizeof(bits); ++index) {
                uint8_t value = bits[index];

                while (value != 0) {
                    value &= static_cast<uint8_t>(value - 1U);
                    ++total;
                }
            }

            return total;
        }

        NEFORCE_NODISCARD bool has_data() const noexcept { return popcount() != 0; }

        NEFORCE_NODISCARD bool same_as(const byte_mask& other) const noexcept {
            return memory_compare(bits, other.bits, sizeof(bits)) == 0;
        }
    };

    struct mask_list {
        byte_mask items[8] = {};
        int count = 0;

        void push(const byte_mask& mask) {
            if (count < 8) {
                items[count] = mask;
                ++count;
            }
        }

        void clear() { count = 0; }
    };

    void collect_required(const int32_t index, mask_list& out) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        switch (entry.type) {
            case node::kind::literal: {
                const auto code_point = static_cast<uint32_t>(entry.code_point);
                byte_mask mask;

                if (code_point < 0x80U) {
                    mask.set(code_point);

                    if (entry.caseless) {
                        if (code_point >= 'a' && code_point <= 'z') {
                            mask.set(code_point - 32U);
                        } else if (code_point >= 'A' && code_point <= 'Z') {
                            mask.set(code_point + 32U);
                        }
                    }
                } else if (code_point <= 0x7FFU) {
                    mask.set(0xC0U | (code_point >> 6U));
                } else if (code_point <= 0xFFFFU) {
                    mask.set(0xE0U | (code_point >> 12U));
                } else {
                    mask.set(0xF0U | (code_point >> 18U));
                }

                out.push(mask);
                break;
            }
            case node::kind::char_class: {
                const char_class& cls = classes_[static_cast<size_t>(entry.class_index)];

                if (cls.negate || cls.caseless) {
                    return;
                }

                byte_mask mask;
                bool ascii_only = true;

                for (const code_range& range: cls.ranges) {
                    if (range.last >= 0x80U) {
                        ascii_only = false;
                        break;
                    }

                    for (uint32_t value = range.first; value <= range.last; ++value) {
                        mask.set(value);
                    }
                }

                if (ascii_only && mask.has_data() && mask.popcount() <= 64) {
                    out.push(mask);
                }
                break;
            }
            case node::kind::concat:
                for (const int32_t child: entry.children) {
                    collect_required(child, out);
                }
                break;
            case node::kind::alternate: {
                if (entry.children.empty()) {
                    return;
                }

                mask_list common;
                collect_required(entry.children[0], common);

                for (size_t branch = 1; branch < entry.children.size() && common.count > 0; ++branch) {
                    mask_list others;
                    collect_required(entry.children[branch], others);
                    mask_list kept;

                    for (int candidate_index = 0; candidate_index < common.count; ++candidate_index) {
                        const byte_mask& candidate = common.items[candidate_index];

                        for (int other_index = 0; other_index < others.count; ++other_index) {
                            if (candidate.same_as(others.items[other_index])) {
                                kept.push(candidate);
                                break;
                            }
                        }
                    }

                    common = kept;
                }

                for (int mask_index = 0; mask_index < common.count; ++mask_index) {
                    out.push(common.items[mask_index]);
                }
                break;
            }
            case node::kind::capture:
            case node::kind::group:
            case node::kind::atomic:
                collect_required(entry.children[0], out);
                break;
            case node::kind::look:
                if (entry.look_kind == 1 || entry.look_kind == 3) {
                    collect_required(entry.children[0], out);
                }
                break;
            case node::kind::repeat:
                if (entry.minimum >= 1) {
                    collect_required(entry.children[0], out);
                }
                break;
            case node::kind::conditional: {
                if (entry.group < 0 || entry.children.size() < 2) {
                    return;
                }

                mask_list common;
                collect_required(entry.children[0], common);
                mask_list others;
                collect_required(entry.children[1], others);

                for (int candidate_index = 0; candidate_index < common.count; ++candidate_index) {
                    for (int other_index = 0; other_index < others.count; ++other_index) {
                        if (common.items[candidate_index].same_as(others.items[other_index])) {
                            out.push(common.items[candidate_index]);
                            break;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    bool emit_single_char_run(const node& repeat) {
        const node& atom = nodes_[static_cast<size_t>(repeat.children[0])];
        const int32_t minimum = repeat.minimum > 0 ? 1 : 0;

        switch (atom.type) {
            case node::kind::literal:
                emit(op::run_char, 0, static_cast<int32_t>(atom.code_point), atom.caseless ? 1 : 0, minimum);
                return true;
            case node::kind::char_class:
                emit(op::run_char, 1, atom.class_index, 0, minimum);
                return true;
            case node::kind::any:
                emit(op::run_char, 2, 0, atom.dotall ? 2 : 0, minimum);
                return true;
            default:
                return false;
        }
    }

    bool emit_greedy_char_loop(const node& repeat) {
        const node& atom = nodes_[static_cast<size_t>(repeat.children[0])];
        int32_t kind = 0;
        int32_t value = 0;
        int32_t flags = 0;

        switch (atom.type) {
            case node::kind::literal:
                kind = 0;
                value = static_cast<int32_t>(atom.code_point);
                flags = atom.caseless ? 1 : 0;
                break;
            case node::kind::char_class:
                kind = 1;
                value = atom.class_index;
                break;
            case node::kind::any:
                kind = 2;
                flags = atom.dotall ? 2 : 0;
                break;
            default:
                return false;
        }

        if (repeat.minimum > 0) {
            emit(op::run_char, kind, value, flags | 4, 1);
        }

        const auto loop = emit(op::repeat_char, 0, 0, flags, kind, value);
        code[static_cast<size_t>(loop)].a = loop;
        const auto after = static_cast<int32_t>(code.size());
        code[static_cast<size_t>(loop)].b = after;
        return true;
    }

    bool emit_fixed_run(const node& repeat) {
        const node& atom = nodes_[static_cast<size_t>(repeat.children[0])];
        const int32_t count = repeat.minimum;

        switch (atom.type) {
            case node::kind::literal:
                emit(op::run_char, 0, static_cast<int32_t>(atom.code_point),
                     static_cast<int32_t>((atom.caseless ? 1 : 0) | 4), count);
                return true;
            case node::kind::char_class:
                emit(op::run_char, 1, atom.class_index, 4, count);
                return true;
            case node::kind::any:
                emit(op::run_char, 2, 0, static_cast<int32_t>((atom.dotall ? 2 : 0) | 4), count);
                return true;
            default:
                return false;
        }
    }

    int32_t unwrap_node(int32_t index) const noexcept {
        for (;;) {
            const node& entry = nodes_[static_cast<size_t>(index)];

            if (entry.type != node::kind::capture && entry.type != node::kind::group &&
                entry.type != node::kind::atomic) {
                return index;
            }

            index = entry.children[0];
        }
    }

    void detect_start_run(const int32_t root) {
        const int32_t body = unwrap_node(root);
        const node& top = nodes_[static_cast<size_t>(body)];

        if (top.type != node::kind::concat || top.children.size() < 2) {
            return;
        }

        const int32_t repeat_index = unwrap_node(top.children[0]);
        const node& repeat = nodes_[static_cast<size_t>(repeat_index)];

        if (repeat.type != node::kind::repeat || repeat.minimum < 1 || repeat.maximum == 0) {
            return;
        }

        const int32_t atom_index = unwrap_node(repeat.children[0]);
        const node& atom = nodes_[static_cast<size_t>(atom_index)];

        if (atom.type != node::kind::char_class) {
            return;
        }

        const int32_t literal_index = unwrap_node(top.children[1]);
        const node& literal = nodes_[static_cast<size_t>(literal_index)];

        if (literal.type != node::kind::literal || literal.caseless || literal.code_point >= 0x80U) {
            return;
        }

        const char_class& cls = classes_[static_cast<size_t>(atom.class_index)];

        if (cls.contains(literal.code_point)) {
            return;
        }

        start_run_class_ = atom.class_index;
        start_run_literal_ = static_cast<int>(literal.code_point);
    }

    void finish_required(const int32_t root) {
        mask_list masks;
        collect_required(root, masks);
        mask_list unique;

        for (int mask_index = 0; mask_index < masks.count; ++mask_index) {
            bool duplicate = false;

            for (int kept_index = 0; kept_index < unique.count; ++kept_index) {
                if (unique.items[kept_index].same_as(masks.items[mask_index])) {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate) {
                unique.push(masks.items[mask_index]);
            }
        }

        for (int i = 1; i < unique.count; ++i) {
            const byte_mask key = unique.items[i];
            int position = i;

            while (position > 0 && unique.items[position - 1].popcount() > key.popcount()) {
                unique.items[position] = unique.items[position - 1];
                --position;
            }

            unique.items[position] = key;
        }

        required_count_ = unique.count < 2 ? unique.count : 2;

        for (int index = 0; index < required_count_; ++index) {
            memory_copy(required_masks_[index], unique.items[index].bits, sizeof(required_masks_[index]));
        }
    }

    struct follow_set {
        bool valid = false;
        byte_bits bits;
    };


    struct risk_state {
        bool has_repeat = false;
        bool backtracking_repeat = false;
    };

    struct measure_entry {
        int64_t minimum = 0;
        int64_t maximum = 0;
        int8_t state = 0;
    };

    vector<follow_set> follow_;
    vector<uint8_t> final_possessive_;
    mutable vector<measure_entry> measure_cache_;
    vector<byte_bits> first_bits_;
    vector<int8_t> first_state_;

    bool first_set_of(const int32_t index, byte_bits& set) {
        const auto slot = static_cast<size_t>(index);

        if (slot < first_state_.size()) {
            if (first_state_[slot] == 2) {
                set = first_bits_[slot];
                return true;
            }

            if (first_state_[slot] == 1) {
                return false;
            }
        }

        const bool saved = start_anchored_;
        start_anchored_ = false;
        const bool result = collect_first(index, set);
        start_anchored_ = saved;

        if (slot < first_state_.size()) {
            first_state_[slot] = result ? 2 : 1;

            if (result) {
                first_bits_[slot] = set;
            }
        }

        return result;
    }

    NEFORCE_NODISCARD bool can_possessify(const int32_t index) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        if (entry.type != node::kind::repeat || entry.mode != node::repeat_mode::greedy) {
            return false;
        }

        if (static_cast<size_t>(index) >= follow_.size() || !follow_[static_cast<size_t>(index)].valid) {
            return false;
        }

        if (node_nullable(entry.children[0])) {
            return false;
        }

        byte_bits child_first;

        if (!first_set_of(entry.children[0], child_first)) {
            return false;
        }

        return !child_first.intersects(follow_[static_cast<size_t>(index)].bits);
    }

    void prepare_follow(const int32_t root) {
        follow_.assign(nodes_.size(), follow_set{});
        final_possessive_.assign(nodes_.size(), 0);
        measure_cache_.assign(nodes_.size(), measure_entry{});
        first_bits_.assign(nodes_.size(), byte_bits{});
        first_state_.assign(nodes_.size(), 0);

        byte_bits end_of_pattern;
        propagate_follow(root, end_of_pattern, true);
    }

    void propagate_follow(const int32_t index, const byte_bits& follow, const bool follow_valid) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        follow_[static_cast<size_t>(index)].valid = follow_valid;

        if (follow_valid) {
            follow_[static_cast<size_t>(index)].bits = follow;
        }

        switch (entry.type) {
            case node::kind::concat: {
                byte_bits pending;
                bool pending_valid = follow_valid;

                if (pending_valid) {
                    pending = follow;
                }

                for (size_t position = entry.children.size(); position > 0; --position) {
                    const int32_t child = entry.children[position - 1];
                    propagate_follow(child, pending, pending_valid);

                    byte_bits child_first;
                    const bool child_first_valid = first_set_of(child, child_first);

                    if (child_first_valid && pending_valid) {
                        pending.merge(child_first);
                    } else if (child_first_valid) {
                        pending = child_first;
                        pending_valid = true;
                    } else {
                        pending_valid = false;
                    }

                    if (!node_nullable(child)) {
                        if (child_first_valid) {
                            pending = child_first;
                            pending_valid = true;
                        } else {
                            pending_valid = false;
                        }
                    }
                }
                break;
            }
            case node::kind::alternate:
                for (const int32_t child: entry.children) {
                    propagate_follow(child, follow, follow_valid);
                }
                break;
            case node::kind::capture:
            case node::kind::group:
            case node::kind::atomic:
                propagate_follow(entry.children[0], follow, follow_valid);
                break;
            case node::kind::repeat: {
                byte_bits child_first;
                const bool child_first_valid = first_set_of(entry.children[0], child_first);

                if (child_first_valid && follow_valid) {
                    child_first.merge(follow);
                    propagate_follow(entry.children[0], child_first, true);
                } else if (child_first_valid) {
                    propagate_follow(entry.children[0], child_first, true);
                } else {
                    propagate_follow(entry.children[0], follow, follow_valid);
                }
                break;
            }
            default:
                break;
        }
    }

    void scan_risk(const int32_t index, risk_state& state) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        if (entry.type == node::kind::repeat) {
            risk_state child;
            scan_risk(entry.children[0], child);

            const bool possessive = final_possessive_[static_cast<size_t>(index)] != 0;

            if (child.has_repeat) {
                nested_quantifier_ = true;

                if (!possessive) {
                    exponential_risk_ = true;
                }
            }

            state.has_repeat = true;
            state.backtracking_repeat = !possessive || child.backtracking_repeat;
            return;
        }

        for (const int32_t child: entry.children) {
            risk_state branch;
            scan_risk(child, branch);
            state.has_repeat = state.has_repeat || branch.has_repeat;
            state.backtracking_repeat = state.backtracking_repeat || branch.backtracking_repeat;
        }
    }
    NEFORCE_NODISCARD bool node_nullable(const int32_t index) const {
        int64_t minimum = 0;
        int64_t maximum = 0;
        measure(index, minimum, maximum);
        return minimum == 0;
    }

    NEFORCE_NODISCARD uint8_t lead_byte(const uint32_t code_point) const noexcept {
        if (!utf_) {
            return static_cast<uint8_t>(code_point & 0xFFU);
        }
        if (code_point < 0x80U) {
            return static_cast<uint8_t>(code_point);
        }
        if (code_point < 0x800U) {
            return static_cast<uint8_t>(0xC0U | (code_point >> 6U));
        }
        if (code_point < 0x10000U) {
            return static_cast<uint8_t>(0xE0U | (code_point >> 12U));
        }
        return static_cast<uint8_t>(0xF0U | (code_point >> 18U));
    }

    void add_range(const uint32_t first, const uint32_t last, byte_bits& set) const {
        const uint8_t low = lead_byte(first);
        const uint8_t high = lead_byte(last);

        for (uint32_t value = low; value <= high; ++value) {
            set.set(value);
        }

        if (first < 0x80U && last >= 0x80U) {
            const uint32_t end = last < 0x7FU ? last : 0x7FU;

            for (uint32_t value = first; value <= end; ++value) {
                set.set(value);
            }
        }
    }

    bool collect_first(const int32_t index, byte_bits& set) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        switch (entry.type) {
            case node::kind::empty:
            case node::kind::cluster:
            case node::kind::backref:
            case node::kind::call:
            case node::kind::word_boundary:
            case node::kind::non_word_boundary:
            case node::kind::begin_line:
            case node::kind::end_line:
            case node::kind::begin_subject:
            case node::kind::end_subject:
            case node::kind::end_subject_final:
            case node::kind::look:
                return false;
            case node::kind::literal:
                set.set(lead_byte(entry.code_point));

                if (entry.caseless) {
                    set.set(lead_byte(to_lower(entry.code_point)));
                    set.set(lead_byte(to_upper(entry.code_point)));
                }

                return true;
            case node::kind::any: {
                const uint32_t upper = utf_ ? 0x7FU : 0xFFU;

                for (uint32_t value = 0; value <= upper; ++value) {
                    if (value != '\n') {
                        set.set(value);
                    }
                }

                if (utf_) {
                    for (uint32_t value = 0xC2U; value <= 0xF4U; ++value) {
                        set.set(value);
                    }
                }

                return true;
            }
            case node::kind::char_class: {
                const char_class& cls = classes_[static_cast<size_t>(entry.class_index)];

                if (cls.negate || cls.caseless) {
                    return false;
                }

                for (const auto& range: cls.ranges) {
                    add_range(range.first, range.last, set);
                }

                return !cls.ranges.empty();
            }
            case node::kind::capture:
            case node::kind::group:
            case node::kind::atomic:
                return collect_first(entry.children[0], set);
            case node::kind::repeat: {
                if (entry.minimum == 0) {
                    return false;
                }

                if (node_nullable(entry.children[0])) {
                    return false;
                }

                return collect_first(entry.children[0], set);
            }
            case node::kind::alternate: {
                bool any = false;

                for (const int32_t child: entry.children) {
                    if (!collect_first(child, set)) {
                        return false;
                    }
                    any = true;
                }

                return any;
            }
            case node::kind::concat: {
                for (const int32_t child: entry.children) {
                    const node& item = nodes_[static_cast<size_t>(child)];

                    if (item.type == node::kind::begin_subject ||
                        (item.type == node::kind::begin_line && !item.multiline)) {
                        start_anchored_ = true;
                        continue;
                    }

                    if (item.type == node::kind::look || item.type == node::kind::word_boundary ||
                        item.type == node::kind::non_word_boundary || item.type == node::kind::end_line ||
                        item.type == node::kind::end_subject || item.type == node::kind::end_subject_final) {
                        continue;
                    }

                    if (!collect_first(child, set)) {
                        return false;
                    }

                    if (!node_nullable(child)) {
                        return true;
                    }
                }

                return false;
            }
            default:
                break;
        }

        return false;
    }

    const vector<node>& nodes_;
    const vector<char_class>& classes_;
    vector<int32_t> capture_entry_;
    vector<call_fixup> call_fixups_;
    bool utf_ = false;
    bool dollar_endonly_ = false;
    bool multiline_circumflex_ = false;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_ENGINE_COMPILER_SRC__
