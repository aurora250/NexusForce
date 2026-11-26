#ifndef NEFORCE_CORE_STRING_REGEX_BUILTIN_HPP__
#define NEFORCE_CORE_STRING_REGEX_BUILTIN_HPP__
#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct regex_program;
struct regex_match;

constexpr uint32_t match_anchored = 0x80000000U;
constexpr uint32_t match_end_anchored = 0x20000000U;

constexpr int match_result_nomatch = -1;
constexpr int match_result_too_complex = -2;
constexpr int match_result_bad_utf = -3;

regex_program* regex_compile(const char* pattern, size_t length, uint32_t options, int* error_code,
                             size_t* error_offset, char* message, size_t message_size);
void regex_free(regex_program* program) noexcept;

int regex_capture_count(const regex_program* program) noexcept;

regex_match* regex_match_create(const regex_program* program);
void regex_match_free(regex_match* data) noexcept;

int regex_exec(const regex_program* program, const char* subject, size_t length, size_t start_offset, uint32_t options,
               regex_match* data);

const size_t* regex_ovector(const regex_match* data) noexcept;

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_BUILTIN_HPP__
