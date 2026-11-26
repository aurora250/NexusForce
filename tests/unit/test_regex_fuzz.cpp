#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/string/string.hpp>
#include <gtest/gtest.h>

using namespace neforce;

namespace {

    class fuzz_random {
    public:
        explicit fuzz_random(const uint64_t seed) :
        state_(seed | 1U) {}

        uint32_t next() {
            state_ ^= state_ << 13U;
            state_ ^= state_ >> 7U;
            state_ ^= state_ << 17U;
            return static_cast<uint32_t>(state_);
        }

        uint32_t below(const uint32_t bound) { return bound == 0 ? 0 : next() % bound; }

        bool chance(const uint32_t percent) { return below(100) < percent; }

    private:
        uint64_t state_;
    };

    void append_sequence(string& out, fuzz_random& random, int depth);

    void append_atom(string& out, fuzz_random& random, const int depth) {
        switch (random.below(10)) {
            case 0:
                out += static_cast<char>('a' + static_cast<int>(random.below(3)));
                break;
            case 1:
                out += '.';
                break;
            case 2:
                out += "\\d";
                break;
            case 3:
                out += "\\w";
                break;
            case 4:
                out += "[a-c]";
                break;
            case 5:
                out += "[^0-9]";
                break;
            case 6:
                out += "\\s";
                break;
            case 7:
                if (depth < 2) {
                    out += "(?=";
                    append_sequence(out, random, depth + 1);
                    out += ')';
                } else {
                    out += 'b';
                }
                break;
            case 8:
                if (depth < 2) {
                    out += "(?:";
                    append_sequence(out, random, depth + 1);
                    out += ')';
                } else {
                    out += 'c';
                }
                break;
            default:
                out += static_cast<char>('0' + static_cast<int>(random.below(4)));
                break;
        }
    }

    void append_quantifier(string& out, fuzz_random& random) {
        switch (random.below(7)) {
            case 0:
                out += '*';
                break;
            case 1:
                out += '+';
                break;
            case 2:
                out += '?';
                break;
            case 3:
                out += "{0,2}";
                break;
            case 4:
                out += "{1,3}";
                break;
            default:
                return;
        }

        if (random.chance(20)) {
            out += random.chance(50) ? '?' : '+';
        }
    }

    void append_sequence(string& out, fuzz_random& random, const int depth) {
        const uint32_t items = 1 + random.below(3);

        for (uint32_t index = 0; index < items; ++index) {
            if (random.chance(10)) {
                out += '^';
            }

            append_atom(out, random, depth);
            append_quantifier(out, random);
        }

        if (random.chance(15)) {
            out += '|';
            append_atom(out, random, depth);
        }
    }

    string build_pattern(fuzz_random& random) {
        string pattern;

        if (random.chance(15)) {
            pattern += '^';
        }

        append_sequence(pattern, random, 0);

        if (random.chance(15)) {
            pattern += '$';
        }

        return pattern;
    }

    regex_option build_options(fuzz_random& random) {
        regex_option options = regex_option::none;

        if (random.chance(25)) {
            options = options | regex_option::caseless;
        }
        if (random.chance(20)) {
            options = options | regex_option::multiline;
        }
        if (random.chance(20)) {
            options = options | regex_option::dotall;
        }
        if (random.chance(30)) {
            options = options | regex_option::utf;
        }
        if (random.chance(20)) {
            options = options | regex_option::ucp;
        }

        return options;
    }

    string build_subject(fuzz_random& random) {
        static const char* const fragments[] = {"a", "b", "c", "0", "1", " ", "\n", "中", "_", "x"};

        const uint32_t length = random.below(24);
        string subject;

        for (uint32_t index = 0; index < length; ++index) {
            subject += fragments[random.below(10)];
        }

        return subject;
    }

    void check_result(const match_result& result, const string& subject) {
        if (!result.matched()) {
            return;
        }

        ASSERT_LE(result.position(), subject.size());
        ASSERT_LE(result.position() + result.length(), subject.size());

        const string_view matched_text = result.data();
        ASSERT_EQ(string(matched_text.data(), matched_text.size()),
                  string(subject.data() + result.position(), result.length()));

        for (size_t index = 0; index < result.size(); ++index) {
            const auto position = result.position(index);

            if (position.first == string::npos) {
                continue;
            }

            ASSERT_LE(position.first, subject.size());
            ASSERT_LE(position.first + position.second, subject.size());
        }
    }

} // namespace

class RegexFuzzTest : public ::testing::Test {};

TEST_F(RegexFuzzTest, RandomPatternsKeepResultInvariants) {
    fuzz_random random(0xC0FFEE1234ULL);

    for (int iteration = 0; iteration < 1500; ++iteration) {
        const string pattern = build_pattern(random);
        const regex_option options = build_options(random);
        const string subject = build_subject(random);

        SCOPED_TRACE(("pattern: " + pattern + " subject: " + subject).data());

        try {
            regex expression(pattern, options);

            check_result(expression.search(subject), subject);

            const auto matches = expression.find_all(subject);
            size_t previous = 0;

            for (const auto& match: matches) {
                check_result(match, subject);
                ASSERT_GE(match.position(), previous);
                previous = match.position();
            }

            const string replaced = expression.replace_all(subject, "#");

            if (matches.empty()) {
                ASSERT_EQ(replaced, subject);
            }

            (void) expression.split(subject);
        } catch (const regex_exception&) {
        }
    }
}
