#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/ini/ini_builder.hpp>
#include <NeForce/core/file/ini/ini_parser.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/utility/packages.hpp>

#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic ignored "-Wstringop-overread"
#endif

using namespace neforce;

namespace {
    void append_index(string& out, size_t value) {
        char buffer[20];
        size_t count = 0;
        do {
            buffer[count++] = static_cast<char>('0' + static_cast<int>(value % 10));
            value /= 10;
        } while (value != 0);
        while (count != 0) {
            out += buffer[--count];
        }
    }

    void append_section(string& out, const size_t index, const size_t fields) {
        out += "[section";
        append_index(out, index);
        out += "]\n";
        for (size_t i = 0; i < fields; ++i) {
            out += "key";
            append_index(out, i);
            out += " = value";
            append_index(out, index * fields + i);
            out += '\n';
        }
    }

    string make_flat_section(const size_t fields) {
        string out;
        out.reserve(fields * 24);
        append_section(out, 0, fields);
        return out;
    }

    string make_sections(const size_t sections, const size_t fields) {
        string out;
        out.reserve(sections * fields * 24);
        for (size_t i = 0; i < sections; ++i) {
            append_section(out, i, fields);
        }
        return out;
    }

    string make_string_values(const size_t fields, const size_t value_len) {
        string out;
        out.reserve(fields * (value_len + 16));
        out += "[strings]\n";
        for (size_t i = 0; i < fields; ++i) {
            out += "name";
            append_index(out, i);
            out += " = ";
            const char fill = static_cast<char>('a' + static_cast<int>(i % 26));
            for (size_t k = 0; k < value_len; ++k) {
                out += fill;
            }
            out += '\n';
        }
        return out;
    }

    string make_global(const size_t fields) {
        string out;
        out.reserve(fields * 20);
        for (size_t i = 0; i < fields; ++i) {
            out += "key";
            append_index(out, i);
            out += " = ";
            append_index(out, i * 3);
            out += '\n';
        }
        return out;
    }

    string make_comment_heavy(const size_t fields) {
        string out;
        out.reserve(fields * 40);
        out += "[comments]\n";
        for (size_t i = 0; i < fields; ++i) {
            out += "; comment for key";
            append_index(out, i);
            out += "\n\nkey";
            append_index(out, i);
            out += " = ";
            append_index(out, i);
            out += '\n';
        }
        return out;
    }

    struct document {
        const char* name = "";
        string text;
    };

    vector<document> documents() {
        vector<document> docs;
        docs.push_back(document{"FlatSection_64", make_flat_section(64)});
        docs.push_back(document{"Sections_128x8", make_sections(128, 8)});
        docs.push_back(document{"StringValues_64x32", make_string_values(64, 32)});
        docs.push_back(document{"GlobalOnly_256", make_global(256)});
        docs.push_back(document{"CommentHeavy_128", make_comment_heavy(128)});
        docs.push_back(document{"LargeSections_32x64", make_sections(32, 64)});
        return docs;
    }

    string build_document(const size_t sections, const size_t fields) {
        ini_builder builder;
        for (size_t s = 0; s < sections; ++s) {
            builder.begin_section("section" + to_string(static_cast<int64_t>(s)));
            for (size_t i = 0; i < fields; ++i) {
                builder.key("key" + to_string(static_cast<int64_t>(i))).value(static_cast<int>(s * fields + i));
            }
            builder.end_section();
        }
        return builder.build().to_string();
    }
} // namespace

static void BM_IniParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        ini_parser parser{string_view(doc.text.data(), doc.text.size())};
        ini_document root = parser.parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_IniParseOwning(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        ini_parser parser{doc.text};
        ini_document root = parser.parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_IniTryParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        ini_parser parser{string_view(doc.text.data(), doc.text.size())};
        optional<ini_document> root = parser.try_parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_IniSerialize(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    const ini_document root = ini_parser{string_view(doc.text.data(), doc.text.size())}.parse();
    size_t produced = 0;
    for (auto _: state) {
        string text = root.to_string();
        benchmark::DoNotOptimize(text);
        produced += text.size();
    }
    state.SetBytesProcessed(static_cast<int64_t>(produced));
    state.SetLabel(doc.name);
}

static void BM_IniRoundTrip(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        ini_document root = ini_parser{string_view(doc.text.data(), doc.text.size())}.parse();
        const string text = root.to_string();
        ini_document again = ini_parser{string_view(text.data(), text.size())}.parse();
        benchmark::DoNotOptimize(again);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_IniBuild(benchmark::State& state) {
    const size_t sections = static_cast<size_t>(state.range(0));
    const size_t fields = static_cast<size_t>(state.range(1));
    for (auto _: state) {
        const string text = build_document(sections, fields);
        benchmark::DoNotOptimize(text);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(sections * fields));
}

static void BM_IniLookup(benchmark::State& state) {
    const size_t sections = static_cast<size_t>(state.range(0));
    const size_t fields = static_cast<size_t>(state.range(1));
    const string text = make_sections(sections, fields);
    const ini_document root = ini_parser{string_view(text.data(), text.size())}.parse();
    const string section_name = "section" + to_string(static_cast<int64_t>(sections / 2));
    const string key_name = "key" + to_string(static_cast<int64_t>(fields / 2));
    for (auto _: state) {
        const ini_section section = root.get_section(section_name);
        const ini_value* value = section.get_property(key_name);
        benchmark::DoNotOptimize(value);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_IniParse)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IniParseOwning)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IniTryParse)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IniSerialize)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IniRoundTrip)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IniBuild)->ArgsProduct({{1, 16, 128}, {64}})->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IniLookup)->ArgsProduct({{8, 128, 1024}, {8}})->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
