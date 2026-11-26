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
        bool first_bytes[256] = {};
        start_valid_ = collect_first(root, first_bytes);

        if (start_valid_) {
            memory_copy(start_bytes_, first_bytes, sizeof(first_bytes));
        }

        prepare_follow(root);

        emit(op::save, 0);
        compile_node(root);
        emit(op::save, 1);
        emit(op::match);

        risk_state state;
        scan_risk(root, state);

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

                if (entry.mode == node::repeat_mode::possessive ||
                    (entry.mode == node::repeat_mode::greedy && can_possessify(index))) {
                    final_possessive_[static_cast<size_t>(index)] = 1;
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
    bool start_valid_ = false;
    bool start_anchored_ = false;
    bool start_bytes_[256] = {};
    bool nested_quantifier_ = false;
    bool exponential_risk_ = false;

private:
    struct follow_set {
        bool valid = false;
        bool bytes[256] = {};
    };

    struct risk_state {
        bool has_repeat = false;
        bool backtracking_repeat = false;
    };

    vector<follow_set> follow_;
    vector<uint8_t> final_possessive_;

    bool first_set_of(const int32_t index, bool* const set) {
        const bool saved = start_anchored_;
        start_anchored_ = false;
        const bool result = collect_first(index, set);
        start_anchored_ = saved;
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

        bool child_first[256] = {};

        if (!first_set_of(entry.children[0], child_first)) {
            return false;
        }

        const follow_set& follow = follow_[static_cast<size_t>(index)];

        for (size_t byte = 0; byte < 256; ++byte) {
            if (child_first[byte] && follow.bytes[byte]) {
                return false;
            }
        }

        return true;
    }

    void prepare_follow(const int32_t root) {
        follow_.assign(nodes_.size(), follow_set{});
        final_possessive_.assign(nodes_.size(), 0);

        bool end_of_pattern[256] = {};
        propagate_follow(root, end_of_pattern, true);
    }

    void propagate_follow(const int32_t index, const bool* const follow, const bool follow_valid) {
        const node& entry = nodes_[static_cast<size_t>(index)];

        follow_[static_cast<size_t>(index)].valid = follow_valid;

        if (follow_valid) {
            memory_copy(follow_[static_cast<size_t>(index)].bytes, follow,
                        sizeof(follow_[static_cast<size_t>(index)].bytes));
        }

        switch (entry.type) {
            case node::kind::concat: {
                bool pending[256] = {};
                bool pending_valid = follow_valid;

                if (pending_valid) {
                    memory_copy(pending, follow, sizeof(pending));
                }

                for (size_t position = entry.children.size(); position > 0; --position) {
                    const int32_t child = entry.children[position - 1];
                    propagate_follow(child, pending, pending_valid);

                    bool child_first[256] = {};
                    const bool child_first_valid = first_set_of(child, child_first);

                    if (child_first_valid && pending_valid) {
                        for (size_t byte = 0; byte < 256; ++byte) {
                            pending[byte] = pending[byte] || child_first[byte];
                        }
                    } else if (child_first_valid) {
                        memory_copy(pending, child_first, sizeof(pending));
                        pending_valid = true;
                    } else {
                        pending_valid = false;
                    }

                    if (!node_nullable(child)) {
                        if (child_first_valid) {
                            memory_copy(pending, child_first, sizeof(pending));
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
                bool child_first[256] = {};
                const bool child_first_valid = first_set_of(entry.children[0], child_first);

                if (child_first_valid && follow_valid) {
                    for (size_t byte = 0; byte < 256; ++byte) {
                        child_first[byte] = child_first[byte] || follow[byte];
                    }
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

    void add_range(const uint32_t first, const uint32_t last, bool* const set) const {
        const uint8_t low = lead_byte(first);
        const uint8_t high = lead_byte(last);

        for (uint32_t value = low; value <= high; ++value) {
            set[value] = true;
        }

        if (first < 0x80U && last >= 0x80U) {
            const uint32_t end = last < 0x7FU ? last : 0x7FU;

            for (uint32_t value = first; value <= end; ++value) {
                set[value] = true;
            }
        }
    }

    bool collect_first(const int32_t index, bool* const set) {
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
                set[lead_byte(entry.code_point)] = true;

                if (entry.caseless) {
                    set[lead_byte(to_lower(entry.code_point))] = true;
                    set[lead_byte(to_upper(entry.code_point))] = true;
                }

                return true;
            case node::kind::any: {
                const uint32_t upper = utf_ ? 0x7FU : 0xFFU;

                for (uint32_t value = 0; value <= upper; ++value) {
                    if (value != '\n') {
                        set[value] = true;
                    }
                }

                if (utf_) {
                    for (uint32_t value = 0xC2U; value <= 0xF4U; ++value) {
                        set[value] = true;
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
