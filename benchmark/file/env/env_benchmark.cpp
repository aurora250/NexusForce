#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/env/env_builder.hpp>
#include <NeForce/core/file/env/env_parser.hpp>
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

    string make_flat(const size_t fields) {
        string out;
        out.reserve(fields * 20);
        for (size_t i = 0; i < fields; ++i) {
            out += "KEY";
            append_index(out, i);
            out += "=value";
            append_index(out, i);
            out += '\n';
        }
        return out;
    }

    string make_string_variables(const size_t fields, const size_t value_len) {
        string out;
        out.reserve(fields * (value_len + 16));
        for (size_t i = 0; i < fields; ++i) {
            out += "NAME";
            append_index(out, i);
            out += "=";
            const char fill = static_cast<char>('a' + static_cast<int>(i % 26));
            for (size_t k = 0; k < value_len; ++k) {
                out += fill;
            }
            out += '\n';
        }
        return out;
    }

    string make_quoted(const size_t fields) {
        string out;
        out.reserve(fields * 28);
        for (size_t i = 0; i < fields; ++i) {
            out += "QUOTED";
            append_index(out, i);
            if (i % 2 == 0) {
                out += "=\"value ";
                append_index(out, i);
                out += "\"\n";
            } else {
                out += "='value ";
                append_index(out, i);
                out += "'\n";
            }
        }
        return out;
    }

    string make_exported(const size_t fields) {
        string out;
        out.reserve(fields * 24);
        for (size_t i = 0; i < fields; ++i) {
            out += "export EXPORTED";
            append_index(out, i);
            out += "=value";
            append_index(out, i);
            out += '\n';
        }
        return out;
    }

    string make_comments(const size_t fields) {
        string out;
        out.reserve(fields * 40);
        for (size_t i = 0; i < fields; ++i) {
            out += "# comment for variable ";
            append_index(out, i);
            out += "\nKEY";
            append_index(out, i);
            out += "=value";
            append_index(out, i);
            out += "\n";
        }
        return out;
    }

    struct document {
        const char* name = "";
        string text;
    };

    vector<document> documents() {
        vector<document> docs;
        docs.push_back(document{"Flat_64", make_flat(64)});
        docs.push_back(document{"Flat_512", make_flat(512)});
        docs.push_back(document{"StringVariables_64x32", make_string_variables(64, 32)});
        docs.push_back(document{"Quoted_128", make_quoted(128)});
        docs.push_back(document{"Exported_128", make_exported(128)});
        docs.push_back(document{"Comments_128", make_comments(128)});
        return docs;
    }

    string build_document(const size_t fields) {
        env_builder builder;
        builder.comment("generated environment");
        for (size_t i = 0; i < fields; ++i) {
            builder.key("KEY" + to_string(static_cast<int64_t>(i))).value("value" + to_string(static_cast<int64_t>(i)));
        }
        return builder.build().to_string();
    }
} // namespace

static void BM_EnvParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        env_parser parser{string_view(doc.text.data(), doc.text.size())};
        env_document root = parser.parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_EnvParseOwning(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        env_parser parser{doc.text};
        env_document root = parser.parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_EnvTryParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        env_parser parser{string_view(doc.text.data(), doc.text.size())};
        optional<env_document> root = parser.try_parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_EnvSerialize(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    const env_document root = env_parser{string_view(doc.text.data(), doc.text.size())}.parse();
    size_t produced = 0;
    for (auto _: state) {
        string text = root.to_string();
        benchmark::DoNotOptimize(text);
        produced += text.size();
    }
    state.SetBytesProcessed(static_cast<int64_t>(produced));
    state.SetLabel(doc.name);
}

static void BM_EnvRoundTrip(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        env_document root = env_parser{string_view(doc.text.data(), doc.text.size())}.parse();
        const string text = root.to_string();
        env_document again = env_parser{string_view(text.data(), text.size())}.parse();
        benchmark::DoNotOptimize(again);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_EnvBuild(benchmark::State& state) {
    const size_t fields = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        const string text = build_document(fields);
        benchmark::DoNotOptimize(text);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

static void BM_EnvLookup(benchmark::State& state) {
    const size_t fields = static_cast<size_t>(state.range(0));
    const string text = make_flat(fields);
    const env_document root = env_parser{string_view(text.data(), text.size())}.parse();
    const string name = "KEY" + to_string(static_cast<int64_t>(fields / 2));
    for (auto _: state) {
        const env_value* value = root.get_variable(name);
        benchmark::DoNotOptimize(value);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_EnvStringExtract(benchmark::State& state) {
    const size_t fields = static_cast<size_t>(state.range(0));
    const string text = make_string_variables(fields, 32);
    const env_document root = env_parser{string_view(text.data(), text.size())}.parse();
    for (auto _: state) {
        size_t total = 0;
        for (const env_member& member: root.get_variables()) {
            total += member.variable().size();
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

BENCHMARK(BM_EnvParse)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EnvParseOwning)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EnvTryParse)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EnvSerialize)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EnvRoundTrip)->DenseRange(0, 5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EnvBuild)->Arg(64)->Arg(512)->Arg(2048)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EnvLookup)->Arg(64)->Arg(512)->Arg(2048)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_EnvStringExtract)->Arg(64)->Arg(512)->Arg(2048)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
