#include <NeForce/core/file/file.hpp>
#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/string/string.hpp>
#include <gtest/gtest.h>

using namespace neforce;

#ifndef NEFORCE_REGEX_CONFORMANCE_FILE
#    define NEFORCE_REGEX_CONFORMANCE_FILE "tests/conformance/regex/cases.txt"
#endif

namespace {

    struct conformance_case {
        string pattern;
        string options;
        string subject;
        string expected;
    };

    string unescape_field(const string_view field) {
        string out;
        out.reserve(field.size());

        for (size_t index = 0; index < field.size(); ++index) {
            if (field[index] == '\\' && index + 1 < field.size()) {
                const char next = field[++index];

                if (next == 't') {
                    out += '\t';
                } else if (next == 'n') {
                    out += '\n';
                } else if (next == 'r') {
                    out += '\r';
                } else {
                    out += next;
                }
            } else {
                out += field[index];
            }
        }

        return out;
    }

    void split_fields(const string_view line, vector<conformance_case>& sink) {
        size_t start = 0;
        vector<string_view> fields;

        while (start <= line.size()) {
            size_t separator = line.find('\t', start);

            if (separator == string::npos) {
                separator = line.size();
            }

            fields.push_back(line.substr(start, separator - start));
            start = separator + 1;
        }

        if (fields.size() != 4) {
            return;
        }

        conformance_case entry;
        entry.pattern = unescape_field(fields[0]);
        entry.options = unescape_field(fields[1]);
        entry.subject = unescape_field(fields[2]);
        entry.expected = string(fields[3].data(), fields[3].size());
        sink.push_back(move(entry));
    }

    vector<conformance_case> load_cases() {
        vector<conformance_case> result;

        const char* const candidates[] = {
                NEFORCE_REGEX_CONFORMANCE_FILE,
                "tests/conformance/regex/cases.txt",
                "../../../tests/conformance/regex/cases.txt",
                "../../../../tests/conformance/regex/cases.txt",
        };

        for (const char* candidate: candidates) {
            try {
                file input{path(candidate)};

                for (auto it = input.begin_lines(); it != file::line_iterator(); ++it) {
                    const string& raw = *it;
                    const size_t length = !raw.empty() && raw[raw.size() - 1] == '\r' ? raw.size() - 1 : raw.size();
                    const string_view line = raw.view(0, length);

                    if (line.empty() || line[0] == '#') {
                        continue;
                    }

                    split_fields(line, result);
                }

                if (!result.empty()) {
                    break;
                }
            } catch (...) {
            }
        }

        return result;
    }

    regex_option to_options(const string& options) {
        regex_option result = regex_option::none;

        for (const char option: options) {
            if (option == 'i') {
                result = result | regex_option::caseless;
            } else if (option == 'm') {
                result = result | regex_option::multiline;
            } else if (option == 's') {
                result = result | regex_option::dotall;
            } else if (option == 'x') {
                result = result | regex_option::extended;
            } else if (option == 'U') {
                result = result | regex_option::ungreedy;
            } else if (option == 'u') {
                result = result | regex_option::utf;
            } else if (option == 'p') {
                result = result | regex_option::ucp;
            } else if (option == 'l') {
                result = result | regex_option::literal;
            } else if (option == 'd') {
                result = result | regex_option::dollar_endonly;
            }
        }

        return result;
    }

    string describe(const match_result& result) {
        if (!result.matched()) {
            return "N";
        }

        string out;

        for (size_t index = 0; index < result.size(); ++index) {
            const auto position = result.position(index);

            if (position.first == string::npos) {
                out += "-;";
            } else {
                out += to_string(position.first) + "," + to_string(position.second) + ";";
            }
        }

        return out;
    }

} // namespace

class RegexConformanceTest : public ::testing::Test {
protected:
    static vector<conformance_case>& cases() {
        static vector<conformance_case> loaded = load_cases();
        return loaded;
    }
};

TEST_F(RegexConformanceTest, CorpusLoaded) { EXPECT_GT(cases().size(), 50u); }

TEST_F(RegexConformanceTest, MatchesRecordedBehaviour) {
    ASSERT_FALSE(cases().empty());

    for (const auto& entry: cases()) {
        SCOPED_TRACE(("pattern: " + entry.pattern + " subject: " + entry.subject).data());

        regex expression(entry.pattern, to_options(entry.options));
        const auto result = expression.search(entry.subject);

        EXPECT_EQ(describe(result), entry.expected);
    }
}
