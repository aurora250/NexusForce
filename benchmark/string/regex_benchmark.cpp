#include <benchmark/benchmark.h>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/string/regex.hpp>
#include <regex>
#include <string>
using namespace neforce;

namespace {

    const char* g_email_pattern = R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})";

    const char* g_text = "Contact us at support@nexusforce.dev or sales@example.com for inquiries. "
                         "For technical issues, reach out to dev-team@neforce.org. "
                         "Personal emails like john.doe+tag@gmail.com are also matched. "
                         "This line has no email address at all. "
                         "admin@subdomain.host.co.uk is a valid address with multiple domain parts. "
                         "Plain text without any at-sign or dot patterns goes here... "
                         "Another one: info@company-name.io and finally ceo@startup.tech.";

    const char* g_url_pattern = R"(https?://[^\s]+)";

    const char* g_csv_line = "123,hello world,42.5,true,optional_field,another_value,7,8,9,end_marker";

    string make_text() { return string(g_text); }

} // namespace

// ============================================================
// 1. Compile pattern
// ============================================================

static void BM_NfRegex_Compile(benchmark::State& state) {
    for (auto _: state) {
        regex re(g_email_pattern);
        benchmark::DoNotOptimize(re);
    }
}
BENCHMARK(BM_NfRegex_Compile);

static void BM_StdRegex_Compile(benchmark::State& state) {
    for (auto _: state) {
        std::regex re(g_email_pattern);
        benchmark::DoNotOptimize(re);
    }
}
BENCHMARK(BM_StdRegex_Compile);

// ============================================================
// 2. Search first match
// ============================================================

static void BM_NfRegex_Search(benchmark::State& state) {
    regex re(g_email_pattern);
    string text = make_text();
    for (auto _: state) {
        auto m = re.search(text);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_NfRegex_Search);

static void BM_StdRegex_Search(benchmark::State& state) {
    std::regex re(g_email_pattern);
    std::string text(g_text);
    for (auto _: state) {
        std::smatch m;
        std::regex_search(text, m, re);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_StdRegex_Search);

// ============================================================
// 3. Find all matches
// ============================================================

static void BM_NfRegex_FindAll(benchmark::State& state) {
    regex re(g_email_pattern);
    string text = make_text();
    for (auto _: state) {
        auto results = re.find_all(text);
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_NfRegex_FindAll);

static void BM_StdRegex_FindAll(benchmark::State& state) {
    std::regex re(g_email_pattern);
    std::string text(g_text);
    for (auto _: state) {
        std::vector<std::smatch> results;
        auto begin = std::sregex_iterator(text.begin(), text.end(), re);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            results.push_back(*it);
        }
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_StdRegex_FindAll);

// ============================================================
// 4. Replace all
// ============================================================

static void BM_NfRegex_ReplaceAll(benchmark::State& state) {
    regex re(g_email_pattern);
    string text = make_text();
    for (auto _: state) {
        auto result = re.replace_all(text, "[REDACTED]");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_NfRegex_ReplaceAll);

static void BM_StdRegex_ReplaceAll(benchmark::State& state) {
    std::regex re(g_email_pattern);
    std::string text(g_text);
    for (auto _: state) {
        auto result = std::regex_replace(text, re, "[REDACTED]");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_StdRegex_ReplaceAll);

// ============================================================
// 5. Split
// ============================================================

static void BM_NfRegex_Split(benchmark::State& state) {
    regex re(",");
    string text(g_csv_line);
    for (auto _: state) {
        auto parts = re.split(text);
        benchmark::DoNotOptimize(parts);
    }
}
BENCHMARK(BM_NfRegex_Split);

static void BM_StdRegex_Split(benchmark::State& state) {
    std::regex re(",");
    std::string text(g_csv_line);
    for (auto _: state) {
        std::vector<std::string> parts;
        auto begin = std::sregex_token_iterator(text.begin(), text.end(), re, -1);
        auto end = std::sregex_token_iterator();
        for (auto it = begin; it != end; ++it) {
            parts.push_back(*it);
        }
        benchmark::DoNotOptimize(parts);
    }
}
BENCHMARK(BM_StdRegex_Split);

// ============================================================
// 6. Full match
// ============================================================

static void BM_NfRegex_Match(benchmark::State& state) {
    regex re(R"(\d{3}-\d{4}-\d{4})");
    string phone("010-1234-5678");
    for (auto _: state) {
        bool m = re.match(phone);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_NfRegex_Match);

static void BM_StdRegex_Match(benchmark::State& state) {
    std::regex re(R"(\d{3}-\d{4}-\d{4})");
    std::string phone("010-1234-5678");
    for (auto _: state) {
        bool m = std::regex_match(phone, re);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_StdRegex_Match);

BENCHMARK_MAIN();
