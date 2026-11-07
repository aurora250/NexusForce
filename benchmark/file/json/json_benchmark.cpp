#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/core/string/format.hpp>

using namespace neforce;

namespace {
    void append_int(string& out, const int64_t value) { format_to(out, "{0}", value); }

    string make_flat_object(const size_t fields) {
        string out;
        out.reserve(fields * 24 + 2);
        out += '{';
        for (size_t i = 0; i < fields; ++i) {
            if (i != 0) {
                out += ',';
            }
            out += R"("key)";
            append_int(out, static_cast<int64_t>(i));
            out += R"(":)";
            append_int(out, static_cast<int64_t>(i * 3));
        }
        out += '}';
        return out;
    }

    string make_string_object(const size_t fields, const size_t value_len) {
        string out;
        out.reserve(fields * (value_len + 24) + 2);
        out += '{';
        for (size_t i = 0; i < fields; ++i) {
            if (i != 0) {
                out += ',';
            }
            out += R"("name)";
            append_int(out, static_cast<int64_t>(i));
            out += R"(":")";
            const char fill = static_cast<char>('a' + static_cast<int>(i % 26));
            for (size_t k = 0; k < value_len; ++k) {
                out += fill;
            }
            out += '"';
        }
        out += '}';
        return out;
    }

    string make_array_of_objects(const size_t items) {
        string out;
        out.reserve(items * 56 + 2);
        out += '[';
        for (size_t i = 0; i < items; ++i) {
            if (i != 0) {
                out += ',';
            }
            out += R"({"id":)";
            append_int(out, static_cast<int64_t>(i));
            out += R"(,"name":"item)";
            append_int(out, static_cast<int64_t>(i));
            out += R"(","value":)";
            append_int(out, static_cast<int64_t>(i));
            out += ".5,\"active\":";
            out += (i % 2 == 0) ? "true" : "false";
            out += '}';
        }
        out += ']';
        return out;
    }

    string make_nested_object(const size_t depth, const size_t breadth) {
        string open;
        string close;
        for (size_t d = 0; d < depth; ++d) {
            open += R"({"level)";
            append_int(open, static_cast<int64_t>(d));
            open += R"(":{"items":[)";
            close = "]}}" + close;
        }
        string body;
        for (size_t i = 0; i < breadth; ++i) {
            if (i != 0) {
                body += ',';
            }
            body += R"({"a":)";
            append_int(body, static_cast<int64_t>(i));
            body += R"(,"b":)";
            append_int(body, static_cast<int64_t>(i));
            body += ".25}";
        }
        string out = open + body + close;
        return out;
    }

    struct document {
        const char* name = "";
        string text;
    };

    vector<document> documents() {
        vector<document> docs;
        docs.emplace_back(document{"FlatObject_32", make_flat_object(32)});
        docs.emplace_back(document{"FlatObject_256", make_flat_object(256)});
        docs.emplace_back(document{"StringObject_64x32", make_string_object(64, 32)});
        docs.emplace_back(document{"ArrayOfObjects_256", make_array_of_objects(256)});
        docs.emplace_back(document{"ArrayOfObjects_2048", make_array_of_objects(2048)});
        docs.emplace_back(document{"Nested_8x32", make_nested_object(8, 32)});
        return docs;
    }

    bool parse_text(const string& text, json_document& doc) noexcept {
        try {
            doc = json_parser{string_view(text.data(), text.size())}.parse();
            return true;
        } catch (...) {
            return false;
        }
    }
} // namespace

static void BM_JsonParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        json_parser parser{string_view(doc.text.data(), doc.text.size())};
        json_document root = parser.parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_JsonTryParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        json_parser parser{string_view(doc.text.data(), doc.text.size())};
        auto root = parser.try_parse();
        benchmark::DoNotOptimize(root);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_JsonSerialize(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    json_document root;
    if (!parse_text(doc.text, root)) {
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

static void BM_JsonSerializeIndent(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    json_document root;
    if (!parse_text(doc.text, root)) {
        state.SkipWithError("parse failed");
        return;
    }
    for (auto _: state) {
        string out = root->to_indented_string();
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_JsonRoundTrip(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        json_parser parser{string_view(doc.text.data(), doc.text.size())};
        auto root = parser.parse();
        string out = root->to_string();
        benchmark::DoNotOptimize(out);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_JsonBuild(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        json_builder builder;
        builder.begin_object();
        for (size_t i = 0; i < fields; ++i) {
            string key = format("key{0}", static_cast<int64_t>(i));
            builder.key(key).value(static_cast<int64_t>(i * 3));
        }
        builder.end_object();
        json_document root = builder.build();
        benchmark::DoNotOptimize(root);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

static void BM_JsonObjectLookup(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    json_document root;
    if (!parse_text(make_flat_object(fields), root)) {
        state.SkipWithError("parse failed");
        return;
    }
    const json_object object = root->as_object();
    const string probe_key = format("key{0}", static_cast<int64_t>(fields / 2));
    for (auto _: state) {
        const json_value* member = object.get_member(probe_key);
        benchmark::DoNotOptimize(member);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_JsonStringExtract(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    json_document root;
    if (!parse_text(make_string_object(fields, 32), root)) {
        state.SkipWithError("parse failed");
        return;
    }
    const json_object object = root->as_object();
    for (auto _: state) {
        size_t total = 0;
        for (const json_member& member: object.get_members()) {
            total += member.value.as_string().get_value().size();
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}


BENCHMARK(BM_JsonParse)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_JsonTryParse)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_JsonSerialize)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_JsonSerializeIndent)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_JsonRoundTrip)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_JsonBuild)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_JsonObjectLookup)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_JsonStringExtract)->Arg(64)->Arg(512)->Arg(2048)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();
