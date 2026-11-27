#ifndef NEFORCE_CORE_STRING_REGEX_ENGINE_BRIDGE_SRC__
#define NEFORCE_CORE_STRING_REGEX_ENGINE_BRIDGE_SRC__
#include "engine_compiler.hpp"
#include "engine_parser.hpp"
#include "engine_vm.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct regex_program;
struct regex_match;

regex_program* regex_compile(const char* pattern, size_t length, uint32_t options, int* error_code,
                             size_t* error_offset, char* message, size_t message_size);

void regex_free(regex_program* program) noexcept;

int regex_capture_count(const regex_program* program) noexcept;

void regex_analysis_of(const regex_program* program, bool* nested, bool* risk) noexcept;

int regex_group_index(const regex_program* program, const char* name, size_t length) noexcept;

regex_match* regex_match_create(const regex_program* program);

void regex_match_free(regex_match* data) noexcept;

int regex_exec(const regex_program* program, const char* subject, size_t length, size_t start_offset, uint32_t options,
               regex_match* data);

const size_t* regex_ovector(const regex_match* data) noexcept;

struct regex_program {
    int references = 1;
    vector<instruction> code;
    vector<char_class> classes;
    int capture_count = 0;
    int loop_count = 0;
    uint32_t options = 0;
    bool has_nested_quantifier = false;
    bool may_backtrack_exponentially = false;
    vector<string> names;
    bool anchored_start = false;
    bool has_start_bitmap = false;
    int required_count = 0;
    int start_run_class = -1;
    int start_run_literal = -1;
    uint8_t start_bitmap[32] = {};
    uint8_t required_masks[2][32] = {};
    uint8_t required_lut[256] = {};
    uint8_t required_full = 0;
};

// The precheck is a single lookup per byte through a byte-to-class map, so it stays cheaper than the scan it
// avoids even for short subjects, and it keeps pathological patterns from spending their whole step budget.
NEFORCE_NODISCARD inline bool regex_required_present(const regex_program* program, const char* subject,
                                                     const size_t length, const size_t start_offset) noexcept {
    if (program == nullptr || program->required_count == 0) {
        return true;
    }

    if (start_offset >= length) {
        return false;
    }

    const uint8_t* const lut = program->required_lut;
    const uint8_t full = program->required_full;
    uint8_t found = 0;

    for (size_t position = start_offset; position < length; ++position) {
        found |= lut[static_cast<uint8_t>(subject[position])];

        if (found == full) {
            return true;
        }
    }

    return false;
}

NEFORCE_NODISCARD inline bool start_bit_set(const regex_program* program, const unsigned char byte) noexcept {
    return (program->start_bitmap[byte >> 3] & static_cast<uint8_t>(1U << (byte & 7U))) != 0;
}

struct regex_match {
    vector<size_t> ovector;
};

inline regex_program* regex_compile(const char* pattern, const size_t length, const uint32_t options, int* error_code,
                                    size_t* error_offset, char* message, const size_t message_size) {
    if (error_code != nullptr) {
        *error_code = 0;
    }
    if (error_offset != nullptr) {
        *error_offset = 0;
    }

    try {
        if ((options & ~accepted_options) != 0U) {
            throw compile_failure{err_unsupported_option, 0};
        }

        parser parsed(pattern, length, options);
        const int32_t root = parsed.parse();

        for (char_class& cls: parsed.classes) {
            cls.build_ascii();
        }

        compiler builder(parsed.nodes, parsed.classes, parsed.capture_count);
        builder.configure(options);
        builder.compile(root);

        auto* program = new regex_program;
        program->code = move(builder.code);
        program->classes = move(parsed.classes);
        program->capture_count = parsed.capture_count;
        program->names = move(parsed.names);
        program->loop_count = builder.loop_count;
        program->options = options;
        program->has_nested_quantifier = builder.nested_quantifier_;
        program->may_backtrack_exponentially = builder.exponential_risk_;
        program->required_count = builder.required_count_;
        program->start_run_class = builder.start_run_class_;
        program->start_run_literal = builder.start_run_literal_;
        memory_copy(program->required_masks, builder.required_masks_, sizeof(program->required_masks));

        if (program->required_count > 0) {
            program->required_full = static_cast<uint8_t>((1U << program->required_count) - 1U);

            for (int mask = 0; mask < program->required_count; ++mask) {
                const auto bit = static_cast<uint8_t>(1U << mask);

                for (size_t bucket = 0; bucket < sizeof(program->required_masks[0]); ++bucket) {
                    auto remaining = static_cast<uintptr_t>(program->required_masks[mask][bucket]);

                    while (remaining != 0) {
                        const auto offset = static_cast<size_t>(countr_zero(remaining));
                        program->required_lut[bucket * 8 + offset] |= bit;
                        remaining &= remaining - 1U;
                    }
                }
            }
        }
        program->anchored_start = builder.start_anchored_;
        program->has_start_bitmap = builder.start_valid_ && !builder.start_anchored_;

        if (program->has_start_bitmap) {
            memory_copy(program->start_bitmap, builder.start_bits_.bits, sizeof(program->start_bitmap));
        }
        return program;
    } catch (const compile_failure& failure) {
        if (error_code != nullptr) {
            *error_code = failure.code;
        }
        if (error_offset != nullptr) {
            *error_offset = failure.offset;
        }
        if (message != nullptr && message_size > 0) {
            const char* text = error_message(failure.code);
            string_copy(message, text, message_size - 1);
            message[message_size - 1] = '\0';
        }
        return nullptr;
    } catch (...) {
        if (error_code != nullptr) {
            *error_code = err_unsupported_feature;
        }
        if (message != nullptr && message_size > 0) {
            string_copy(message, error_message(err_unsupported_feature), message_size - 1);
            message[message_size - 1] = '\0';
        }
        return nullptr;
    }
}

struct program_cache_slot {
    string pattern;
    uint32_t options = 0;
    regex_program* program = nullptr;
    unsigned long long stamp = 0;
};

struct program_cache {
    static constexpr size_t capacity = 8;

    program_cache_slot slots[capacity];
    unsigned long long clock = 0;

    ~program_cache() {
        for (program_cache_slot& slot: slots) {
            if (slot.program != nullptr) {
                regex_free(slot.program);
                slot.program = nullptr;
            }
        }
    }
};

inline program_cache& program_cache_of() noexcept {
    thread_local program_cache cache;
    return cache;
}

inline regex_program* regex_compile_cached(const char* pattern, const size_t length, const uint32_t options,
                                           int* error_code, size_t* error_offset, char* message,
                                           const size_t message_size) {
    program_cache& cache = program_cache_of();
    ++cache.clock;

    for (program_cache_slot& slot: cache.slots) {
        if (slot.program != nullptr && slot.options == options && slot.pattern.length() == length &&
            memory_compare(slot.pattern.data(), pattern, length) == 0) {
            slot.stamp = cache.clock;
            ++slot.program->references;
            return slot.program;
        }
    }

    regex_program* program = regex_compile(pattern, length, options, error_code, error_offset, message, message_size);

    if (program == nullptr) {
        return nullptr;
    }

    program_cache_slot* victim = &cache.slots[0];

    for (program_cache_slot& slot: cache.slots) {
        if (slot.program == nullptr) {
            victim = &slot;
            break;
        }

        if (slot.stamp < victim->stamp) {
            victim = &slot;
        }
    }

    if (victim->program != nullptr) {
        regex_free(victim->program);
    }

    victim->pattern.assign(pattern, length);
    victim->options = options;
    victim->program = program;
    victim->stamp = cache.clock;
    ++program->references;

    return program;
}

inline void regex_free(regex_program* program) noexcept {
    if (program == nullptr || --program->references > 0) {
        return;
    }

    delete program;
}

inline int regex_group_index(const regex_program* program, const char* const name, const size_t length) noexcept {
    if (program == nullptr || name == nullptr || length == 0) {
        return -1;
    }

    for (size_t index = 1; index < program->names.size(); ++index) {
        const string& candidate = program->names[index];

        if (candidate.size() == length && memory_compare(candidate.data(), name, length) == 0) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

inline void regex_analysis_of(const regex_program* program, bool* const nested, bool* const risk) noexcept {
    if (nested != nullptr) {
        *nested = program != nullptr && program->has_nested_quantifier;
    }

    if (risk != nullptr) {
        *risk = program != nullptr && program->may_backtrack_exponentially;
    }
}

inline int regex_capture_count(const regex_program* program) noexcept {
    return program != nullptr ? program->capture_count : 0;
}

struct exec_context {
    match_state state;
    regex_match match;
};

inline exec_context& exec_context_of() noexcept {
    thread_local exec_context context;
    return context;
}

inline void reset_offsets(vector<size_t>& values, const size_t count) {
    if (values.size() != count) {
        values.resize(count);
    }

    for (size_t index = 0; index < count; ++index) {
        values[index] = invalid_offset;
    }
}

inline regex_match* regex_match_create(const regex_program* program) {
    if (program == nullptr) {
        return nullptr;
    }

    regex_match& match = exec_context_of().match;

    const size_t slots = (static_cast<size_t>(program->capture_count) + 1) * 2;

    if (match.ovector.size() != slots) {
        match.ovector.resize(slots);
    }

    return &match;
}

inline void regex_match_free(regex_match* /*unused*/) noexcept {}

inline int regex_exec(const regex_program* program, const char* subject, const size_t length, const size_t start_offset,
                      const uint32_t options, regex_match* data) {
    if (program == nullptr || data == nullptr || subject == nullptr) {
        return match_result_nomatch;
    }

    const size_t capture_slots = (static_cast<size_t>(program->capture_count) + 1) * 2;

    match_state& state = exec_context_of().state;
    state.code = &program->code;
    state.classes = &program->classes;
    state.subject = subject;
    state.length = length;
    state.search_start = start_offset <= length ? start_offset : length;
    state.utf = (program->options & opt_utf) != 0;
    state.ucp = (program->options & opt_ucp) != 0;
    state.match_unset_backref = (program->options & opt_match_unset_backref) != 0;
    reset_offsets(state.slots, capture_slots);

    if (state.loops.size() != static_cast<size_t>(program->loop_count)) {
        state.loops.resize(static_cast<size_t>(program->loop_count));
    }

    for (size_t& loop: state.loops) {
        loop = 0;
    }

    state.undo_index.clear();
    state.undo_value.clear();
    state.frames.clear();
    state.calls.clear();
    ssize_t steps = 0;
    state.steps = 0;
    state.error = 0;

    const bool anchored = (options & match_anchored) != 0;
    const bool end_anchored = (options & match_end_anchored) != 0;
    const bool first_line = (program->options & opt_firstline) != 0;
    const bool single_start = anchored || program->anchored_start;

    size_t start = start_offset <= length ? start_offset : length;

    if (program->anchored_start && start_offset != 0) {
        return match_result_nomatch;
    }

    while (!single_start || start == start_offset) {
        if (program->has_start_bitmap) {
            while (start < length && !start_bit_set(program, static_cast<unsigned char>(subject[start]))) {
                ++start;
            }

            if (start >= length) {
                break;
            }
        }

        if (program->start_run_class >= 0 && start < length) {
            const char_class& run_class = program->classes[static_cast<size_t>(program->start_run_class)];

            if (run_class.contains(static_cast<unsigned char>(subject[start]))) {
                size_t run_end = start;

                while (run_end < length && run_class.contains(static_cast<unsigned char>(subject[run_end]))) {
                    ++run_end;
                }

                if (run_end >= length || static_cast<unsigned char>(subject[run_end]) !=
                                                 static_cast<unsigned char>(program->start_run_literal)) {
                    start = run_end;
                    continue;
                }
            }
        }

        if (first_line) {
            bool stopped = false;

            for (size_t index = 0; index < start; ++index) {
                if (subject[index] == '\n') {
                    stopped = true;
                    break;
                }
            }

            if (stopped) {
                break;
            }
        }

        state.attempt_limit = steps + attempt_steps < maximum_steps ? steps + attempt_steps : maximum_steps;

        if (!state.utf || is_boundary(state, start)) {
            const size_t mark = state.undo_index.size();
            size_t end = 0;

            if (run(state, 0, start, end, steps)) {
                if (!end_anchored || end == length) {
                    data->ovector.assign(capture_slots, invalid_offset);

                    int highest = 0;
                    for (size_t group = 0; group < capture_slots / 2; ++group) {
                        data->ovector[group * 2] = state.slots[group * 2];
                        data->ovector[group * 2 + 1] = state.slots[group * 2 + 1];

                        if (state.slots[group * 2] != invalid_offset) {
                            highest = static_cast<int>(group);
                        }
                    }

                    return highest + 1;
                }

                rollback(state, mark);
            } else if (state.error != 0) {
                return state.error;
            }
        }

        if (start >= length) {
            break;
        }

        size_t step = 1;

        if (state.utf) {
            uint32_t code_point = 0;
            const size_t used = decode_at(state, start, code_point);
            step = used == 0 ? 1 : used;
        }

        start += step;
    }

    return match_result_nomatch;
}

inline const size_t* regex_ovector(const regex_match* data) noexcept {
    return data != nullptr ? data->ovector.data() : nullptr;
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_ENGINE_BRIDGE_SRC__
