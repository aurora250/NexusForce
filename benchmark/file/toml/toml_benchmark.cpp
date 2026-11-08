#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/toml/toml_builder.hpp>
#include <NeForce/core/file/toml/toml_parser.hpp>

#ifdef NEFORCE_COMPILER_GCC
// GCC 13 reports a false positive when appending to an SSO string inside the document generators.
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

    string make_flat_table(const size_t fields) {
        string out;
        out.reserve(fields * 16);
        for (size_t i = 0; i < fields; ++i) {
            out += "key";
            append_index(out, i);
            out += " = ";
            append_index(out, i * 3);
            out += '\n';
        }
        return out;
    }

    string make_string_table(const size_t fields, const size_t value_len) {
        string out;
        out.reserve(fields * (value_len + 16));
        for (size_t i = 0; i < fields; ++i) {
            out += "name";
            append_index(out, i);
            out += " = \"";
            const char fill = static_cast<char>('a' + static_cast<int>(i % 26));
            for (size_t k = 0; k < value_len; ++k) {
                out += fill;
            }
            out += "\"\n";
        }
        return out;
    }

    string make_array_of_tables(const size_t items) {
        string out;
        out.reserve(items * 80);
        for (size_t i = 0; i < items; ++i) {
            out += "[[items]]\nid = ";
            append_index(out, i);
            out += "\nname = \"item";
            append_index(out, i);
            out += "\"\nvalue = ";
            append_index(out, i);
            out += ".5\nactive = ";
            out += i % 2 == 0 ? "true" : "false";
            out += '\n';
        }
        return out;
    }

    string make_nested_tables(const size_t depth, const size_t breadth) {
        string out;
        for (size_t d = 0; d < depth; ++d) {
            for (size_t i = 0; i < breadth; ++i) {
                out += "[[";
                for (size_t level = 0; level <= d; ++level) {
                    if (level != 0) {
                        out += '.';
                    }
                    out += "level";
                    append_index(out, level);
                }
                out += ".items]]\na = ";
                append_index(out, i);
                out += "\nb = ";
                append_index(out, i);
                out += ".25\n";
            }
        }
        return out;
    }

    struct document {
        const char* name = "";
        string text;
    };

    vector<document> documents() {
        vector<document> docs;
        docs.push_back(document{"FlatTable_32", make_flat_table(32)});
        docs.push_back(document{"FlatTable_256", make_flat_table(256)});
        docs.push_back(document{"StringTable_64x32", make_string_table(64, 32)});
        docs.push_back(document{"ArrayOfTables_256", make_array_of_tables(256)});
        docs.push_back(document{"ArrayOfTables_2048", make_array_of_tables(2048)});
        docs.push_back(document{"NestedTables_8x32", make_nested_tables(8, 32)});
        return docs;
    }

    const toml_value* parse_text(const string& text, toml_document& doc) noexcept {
        try {
            doc = toml_parser(string_view(text.data(), text.size())).parse();
            return doc.get();
        } catch (...) {
            return nullptr;
        }
    }
} // namespace

static void BM_TomlParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        toml_parser parser{string_view(doc.text.data(), doc.text.size())};
        toml_document root = parser.parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_TomlParseOwning(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        toml_parser parser{doc.text};
        toml_document root = parser.parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_TomlTryParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        toml_parser parser{string_view(doc.text.data(), doc.text.size())};
        optional<toml_document> root = parser.try_parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_TomlSerialize(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    toml_document root;
    if (parse_text(doc.text, root) == nullptr) {
        state.SkipWithError("parse failed");
        return;
    }
    for (auto _: state) {
        string out = root->to_document();
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_TomlSerializeInline(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    toml_document root;
    if (parse_text(doc.text, root) == nullptr) {
        state.SkipWithError("parse failed");
        return;
    }
    for (auto _: state) {
        string out = root->to_string();
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_TomlRoundTrip(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        toml_document root = toml_parser{string_view(doc.text.data(), doc.text.size())}.parse();
        string out = root->to_document();
        benchmark::DoNotOptimize(out);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_TomlBuild(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        toml_builder builder;
        for (size_t i = 0; i < fields; ++i) {
            string key = format("key{0}", static_cast<int64_t>(i));
            builder.key(move(key)).value(static_cast<int64_t>(i * 3));
        }
        toml_document root = builder.build();
        benchmark::DoNotOptimize(root);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

static void BM_TomlBuildNested(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        toml_builder builder;
        builder.begin_table("database");
        builder.key("host").value("localhost");
        builder.key("port").value(static_cast<int64_t>(5432));
        builder.end_table();
        for (size_t i = 0; i < fields; ++i) {
            builder.begin_array_table("items");
            builder.key("id").value(static_cast<int64_t>(i));
            builder.key("active").value(i % 2 == 0);
            builder.end_array_table();
        }
        toml_document root = builder.build();
        benchmark::DoNotOptimize(root);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

static void BM_TomlLookup(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    toml_document root;
    if (parse_text(make_flat_table(fields), root) == nullptr) {
        state.SkipWithError("parse failed");
        return;
    }
    const toml_table table = root->as_table();
    const string probe_key = format("key{0}", static_cast<int64_t>(fields / 2));
    for (auto _: state) {
        const toml_value* member = table.get_member(probe_key);
        benchmark::DoNotOptimize(member);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_TomlStringExtract(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    toml_document root;
    if (parse_text(make_string_table(fields, 32), root) == nullptr) {
        state.SkipWithError("parse failed");
        return;
    }
    const toml_table table = root->as_table();
    for (auto _: state) {
        size_t total = 0;
        for (const toml_member& member: table.get_members()) {
            total += member.value.as_string().size();
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

BENCHMARK(BM_TomlParse)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlParseOwning)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlTryParse)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlSerialize)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlSerializeInline)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlRoundTrip)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlBuild)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlBuildNested)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TomlLookup)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_TomlStringExtract)->Arg(64)->Arg(512)->Arg(2048)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();
