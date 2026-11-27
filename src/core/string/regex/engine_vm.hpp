#ifndef NEFORCE_CORE_STRING_REGEX_ENGINE_VM_SRC__
#define NEFORCE_CORE_STRING_REGEX_ENGINE_VM_SRC__
#include "engine_common.hpp"
#include "engine_types.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct match_state {
    const vector<instruction>* code = nullptr;
    const vector<char_class>* classes = nullptr;
    const char* subject = nullptr;
    size_t length = 0;
    size_t search_start = 0;
    bool utf = false;
    bool ucp = false;
    bool match_unset_backref = false;
    vector<size_t> slots;
    vector<size_t> loops;
    struct call_frame {
        int32_t return_ip;
        vector<size_t> saved_loops;
        size_t saved_start;
        size_t saved_end;
    };

    struct frame {
        int32_t ip;
        size_t pos;
        size_t undo;
        size_t ret;
    };

    vector<call_frame> calls;
    vector<size_t> undo_index;
    vector<size_t> undo_value;
    vector<frame> frames;
    ssize_t steps = 0;
    ssize_t attempt_limit = attempt_steps;
    int error = 0;
};

NEFORCE_NODISCARD inline size_t decode_at(const match_state& state, const size_t pos, uint32_t& code_point) noexcept {
    if (pos >= state.length) {
        return 0;
    }

    if (!state.utf) {
        code_point = static_cast<unsigned char>(state.subject[pos]);
        return 1;
    }

    return utf8_decode(reinterpret_cast<const unsigned char*>(state.subject) + pos, state.length - pos, code_point);
}

NEFORCE_NODISCARD inline bool is_boundary(const match_state& state, const size_t pos) noexcept {
    if (!state.utf || pos == 0 || pos >= state.length) {
        return true;
    }

    return !is_utf8_trail(static_cast<unsigned char>(state.subject[pos]));
}

NEFORCE_NODISCARD inline uint32_t fold_code_point(const match_state& state, const uint32_t code_point) noexcept {
    if (!state.ucp) {
        return (code_point >= 'A' && code_point <= 'Z') ? code_point + 32 : code_point;
    }

    return to_lower(code_point);
}

NEFORCE_NODISCARD inline bool is_word_at(const match_state& state, const size_t pos, const bool ucp) noexcept {
    uint32_t code_point = 0;

    if (decode_at(state, pos, code_point) == 0) {
        return false;
    }

    return ucp ? is_unicode_word(code_point) : is_ascii_word(code_point);
}

NEFORCE_NODISCARD inline bool is_word_before(const match_state& state, const size_t pos, const bool ucp) noexcept {
    if (pos == 0) {
        return false;
    }

    size_t start = pos - 1;

    if (state.utf) {
        while (start > 0 && is_utf8_trail(static_cast<unsigned char>(state.subject[start]))) {
            --start;
        }
    }

    return is_word_at(state, start, ucp);
}

inline void rollback(match_state& state, const size_t mark) {
    while (state.undo_index.size() > mark) {
        const size_t index = state.undo_index.back();
        const size_t value = state.undo_value.back();
        state.undo_index.pop_back();
        state.undo_value.pop_back();

        if (index < state.slots.size()) {
            state.slots[index] = value;
        } else {
            state.loops[index - state.slots.size()] = value;
        }
    }
}

inline void unwind_calls(match_state& state, const size_t mark) {
    while (state.calls.size() > mark) {
        state.loops = move(state.calls.back().saved_loops);
        state.calls.pop_back();
    }
}

inline void assign_slot(match_state& state, const size_t index, const size_t value) {
    state.undo_index.push_back(index);

    if (index < state.slots.size()) {
        state.undo_value.push_back(state.slots[index]);
        state.slots[index] = value;
    } else {
        const size_t loop = index - state.slots.size();
        state.undo_value.push_back(state.loops[loop]);
        state.loops[loop] = value;
    }
}

NEFORCE_NODISCARD inline bool is_grapheme_extend(const uint32_t code_point) noexcept {
    const uint16_t category = category_of(code_point);
    return category >= 6 && category <= 8;
}

NEFORCE_NODISCARD inline size_t cluster_length(const match_state& state, const size_t pos) noexcept {
    uint32_t code_point = 0;
    const size_t used = decode_at(state, pos, code_point);

    if (used == 0) {
        return 0;
    }

    size_t end = pos + used;

    if (code_point == 0x0DU && end < state.length && state.subject[end] == '\n') {
        return 2;
    }

    if (code_point >= 0x1100U && code_point <= 0x115FU) {
        uint32_t next = 0;
        const size_t next_used = decode_at(state, end, next);

        if (next_used != 0 && next >= 0x1160U && next <= 0x11A7U) {
            end += next_used;
        }
    }

    for (;;) {
        uint32_t next = 0;
        const size_t next_used = decode_at(state, end, next);

        if (next_used == 0) {
            break;
        }

        if (is_grapheme_extend(next)) {
            end += next_used;
            continue;
        }

        if (next == 0x200DU) {
            uint32_t joined = 0;
            const size_t joined_used = decode_at(state, end + next_used, joined);

            if (joined_used == 0) {
                break;
            }

            end += next_used + joined_used;
            continue;
        }

        break;
    }

    return end - pos;
}

bool run(match_state& state, int32_t entry_ip, size_t start_pos, size_t& end_pos, ssize_t& steps);

inline bool lookbehind_check(match_state& state, const size_t pos, const int32_t kind, const int32_t sub,
                             const size_t minimum, const size_t maximum, ssize_t& steps) {
    if (pos < minimum) {
        return kind == 4;
    }

    const size_t low = pos >= maximum ? pos - maximum : 0;
    const size_t high = pos - minimum;

    for (size_t start = low;; ++start) {
        if (!state.utf || is_boundary(state, start)) {
            const size_t mark = state.undo_index.size();
            size_t end = 0;

            if (run(state, sub, start, end, steps) && end == pos) {
                if (kind == 4) {
                    rollback(state, mark);
                    return false;
                }
                return true;
            }

            rollback(state, mark);
        }

        if (start >= high) {
            break;
        }
    }

    return kind == 4;
}

inline bool run(match_state& state, const int32_t entry_ip, const size_t start_pos, size_t& end_pos, ssize_t& steps) {
    // Hoisted out of the dispatch loop: the bytecode array never moves while an attempt is running.
    const instruction* const code = state.code->data();
    const ssize_t limit = state.attempt_limit;

    const size_t base = state.frames.size();
    const size_t entry_undo = state.undo_index.size();
    const size_t entry_calls = state.calls.size();

    int32_t ip = entry_ip;
    size_t pos = start_pos;

    for (;;) {
        if (++steps > limit) {
            state.error = match_result_too_complex;
            break;
        }

        const instruction& current = code[static_cast<size_t>(ip)];

        switch (current.code) {
            case op::fail:
                goto backtrack;
            case op::match:
                if (!state.calls.empty()) {
                    match_state::call_frame frame = move(state.calls.back());
                    state.calls.pop_back();
                    ip = frame.return_ip;
                    assign_slot(state, 0, frame.saved_start);
                    assign_slot(state, 1, frame.saved_end);
                    state.loops = move(frame.saved_loops);
                } else {
                    end_pos = pos;
                    state.frames.resize(base);
                    return true;
                }
                break;
            case op::accept:
                end_pos = pos;
                unwind_calls(state, entry_calls);
                state.frames.resize(base);
                return true;
            case op::char_match: {
                uint32_t code_point = 0;
                const size_t used = decode_at(state, pos, code_point);

                if (used == 0) {
                    goto backtrack;
                }

                const auto expected = static_cast<uint32_t>(current.a);
                bool equal = code_point == expected;

                if (!equal && current.b != 0) {
                    equal = fold_code_point(state, code_point) == fold_code_point(state, expected);
                }

                if (!equal) {
                    goto backtrack;
                }

                pos += used;
                ++ip;
                break;
            }
            case op::any: {
                uint32_t code_point = 0;
                const size_t used = decode_at(state, pos, code_point);

                if (used == 0) {
                    goto backtrack;
                }

                if (current.b == 0 && code_point == '\n') {
                    goto backtrack;
                }

                pos += used;
                ++ip;
                break;
            }
            case op::run_char: {
                size_t count = 0;
                const bool exact = (current.c & 4) != 0;

                while (count < (exact ? static_cast<size_t>(current.d) : state.length) && pos < state.length) {
                    uint32_t code_point = 0;
                    const size_t used = decode_at(state, pos, code_point);

                    if (used == 0) {
                        break;
                    }

                    bool hit = false;

                    switch (current.a) {
                        case 0: {
                            const auto expected = static_cast<uint32_t>(current.b);
                            hit = code_point == expected;

                            if (!hit && (current.c & 1) != 0) {
                                hit = fold_code_point(state, code_point) == fold_code_point(state, expected);
                            }
                            break;
                        }
                        case 1: {
                            const char_class& cls = (*state.classes)[static_cast<size_t>(current.b)];
                            hit = cls.contains(code_point);
                            break;
                        }
                        default:
                            hit = (current.c & 2) != 0 || code_point != '\n';
                            break;
                    }

                    if (!hit) {
                        break;
                    }

                    pos += used;
                    ++count;
                }

                if (exact ? count != static_cast<size_t>(current.d) : (current.d != 0 && count == 0)) {
                    goto backtrack;
                }

                ++ip;
                break;
            }
            case op::repeat_char: {
                if (pos >= state.length) {
                    ip = current.b;
                    break;
                }

                uint32_t code_point = 0;
                const size_t used = decode_at(state, pos, code_point);

                if (used == 0) {
                    ip = current.b;
                    break;
                }

                bool hit = false;

                switch (current.d) {
                    case 0: {
                        const auto expected = static_cast<uint32_t>(current.e);
                        hit = code_point == expected;

                        if (!hit && (current.c & 1) != 0) {
                            hit = fold_code_point(state, code_point) == fold_code_point(state, expected);
                        }
                        break;
                    }
                    case 1: {
                        const char_class& cls = (*state.classes)[static_cast<size_t>(current.e)];
                        hit = cls.contains(code_point);
                        break;
                    }
                    default:
                        hit = (current.c & 2) != 0 || code_point != '\n';
                        break;
                }

                if (!hit) {
                    ip = current.b;
                    break;
                }

                state.frames.push_back(match_state::frame{current.b, pos, state.undo_index.size(), state.calls.size()});
                pos += used;
                ip = current.a;
                break;
            }
            case op::char_class: {
                uint32_t code_point = 0;
                const size_t used = decode_at(state, pos, code_point);

                if (used == 0) {
                    goto backtrack;
                }

                if (!(*state.classes)[static_cast<size_t>(current.a)].contains(code_point)) {
                    goto backtrack;
                }

                pos += used;
                ++ip;
                break;
            }
            case op::split:
                state.frames.push_back(match_state::frame{current.b, pos, state.undo_index.size(), state.calls.size()});
                ip = current.a;
                break;
            case op::jump:
                ip = current.a;
                break;
            case op::save:
                assign_slot(state, static_cast<size_t>(current.a), pos);
                ++ip;
                break;
            case op::begin_line: {
                bool matched = pos == 0;

                if (!matched && current.a != 0) {
                    matched = state.subject[pos - 1] == '\n';
                }

                if (current.b != 0 && pos != 0) {
                    matched = false;
                }

                if (!matched) {
                    goto backtrack;
                }

                ++ip;
                break;
            }
            case op::end_line: {
                bool matched = false;

                if (current.c != 0) {
                    matched = pos == state.length;
                } else if (current.a != 0) {
                    matched = pos == state.length || state.subject[pos] == '\n';
                } else {
                    matched = pos == state.length || (pos + 1 == state.length && state.subject[pos] == '\n');
                }

                if (!matched) {
                    goto backtrack;
                }

                ++ip;
                break;
            }
            case op::begin_subject:
                if (pos != 0) {
                    goto backtrack;
                }
                ++ip;
                break;
            case op::end_subject:
                if (pos != state.length) {
                    goto backtrack;
                }
                ++ip;
                break;
            case op::end_subject_final:
                if (pos != state.length && (pos + 1 != state.length || state.subject[pos] != '\n')) {
                    goto backtrack;
                }
                ++ip;
                break;
            case op::word_boundary:
                if (is_word_before(state, pos, current.a != 0) == is_word_at(state, pos, current.a != 0)) {
                    goto backtrack;
                }
                ++ip;
                break;
            case op::non_word_boundary:
                if (is_word_before(state, pos, current.a != 0) != is_word_at(state, pos, current.a != 0)) {
                    goto backtrack;
                }
                ++ip;
                break;
            case op::backref: {
                const auto group = static_cast<size_t>(current.a);

                if (group * 2 + 1 >= state.slots.size()) {
                    goto backtrack;
                }

                const auto begin = state.slots[group * 2];
                const auto finish = state.slots[group * 2 + 1];

                if (begin == invalid_offset || finish == invalid_offset) {
                    if (!state.match_unset_backref) {
                        goto backtrack;
                    }
                    ++ip;
                    break;
                }

                size_t left = begin;
                size_t right = pos;

                while (left < finish) {
                    uint32_t subject_point = 0;
                    uint32_t group_point = 0;
                    const size_t subject_used = decode_at(state, right, subject_point);
                    const size_t group_used = decode_at(state, left, group_point);

                    if (subject_used == 0 || group_used == 0) {
                        goto backtrack;
                    }

                    if (subject_point != group_point &&
                        (current.b == 0 ||
                         fold_code_point(state, subject_point) != fold_code_point(state, group_point))) {
                        goto backtrack;
                    }

                    left += group_used;
                    right += subject_used;
                }

                pos = right;
                ++ip;
                break;
            }
            case op::look: {
                const int32_t kind = current.b;

                if (kind == 1 || kind == 2) {
                    const size_t mark = state.undo_index.size();
                    size_t end = 0;
                    const bool matched = run(state, current.a, pos, end, steps);

                    if (kind == 1) {
                        if (!matched) {
                            goto backtrack;
                        }
                    } else if (matched) {
                        rollback(state, mark);
                        goto backtrack;
                    }
                } else if (!lookbehind_check(state, pos, kind, current.a, static_cast<size_t>(current.c),
                                             static_cast<size_t>(current.d), steps)) {
                    goto backtrack;
                }

                ip = current.e;
                break;
            }
            case op::atomic: {
                size_t end = 0;

                if (!run(state, current.a, pos, end, steps)) {
                    goto backtrack;
                }

                pos = end;
                ip = current.e;
                break;
            }
            case op::call:
                if (state.calls.size() >= static_cast<size_t>(maximum_recursion)) {
                    state.error = match_result_too_complex;
                    goto abort;
                }
                state.calls.push_back(match_state::call_frame{ip + 1, state.loops, state.slots[0], state.slots[1]});
                ip = current.a;
                break;
            case op::cluster: {
                const size_t used = cluster_length(state, pos);

                if (used == 0) {
                    goto backtrack;
                }

                pos += used;
                ++ip;
                break;
            }
            case op::keep:
                assign_slot(state, 0, pos);
                ++ip;
                break;
            case op::begin_search:
                if (pos != state.search_start) {
                    goto backtrack;
                }
                ++ip;
                break;
            case op::branch_group:
                ip = state.slots[static_cast<size_t>(current.a) * 2] != invalid_offset ? current.b : current.c;
                break;
            case op::loop_enter:
                assign_slot(state, state.slots.size() + static_cast<size_t>(current.a), pos);
                ++ip;
                break;
            case op::loop_check:
                if (state.loops[static_cast<size_t>(current.a)] == pos) {
                    ip = current.b;
                } else {
                    ++ip;
                }
                break;
        }

        continue;

    backtrack:
        if (state.frames.size() <= base) {
            rollback(state, entry_undo);
            unwind_calls(state, entry_calls);
            return false;
        }

        const match_state::frame frame = state.frames.back();
        state.frames.pop_back();
        rollback(state, frame.undo);
        unwind_calls(state, frame.ret);
        ip = frame.ip;
        pos = frame.pos;
    }

abort:
    rollback(state, entry_undo);
    unwind_calls(state, entry_calls);
    state.frames.resize(base);
    return false;
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_ENGINE_VM_SRC__
