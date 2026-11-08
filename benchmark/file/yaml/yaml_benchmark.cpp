#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/yaml/yaml_builder.hpp>
#include <NeForce/core/file/yaml/yaml_parser.hpp>

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

    string make_flat_mapping(const size_t fields) {
        string out;
        out.reserve(fields * 16);
        for (size_t i = 0; i < fields; ++i) {
            out += "key";
            append_index(out, i);
            out += ": ";
            append_index(out, i * 3);
            out += '\n';
        }
        return out;
    }

    string make_string_mapping(const size_t fields, const size_t value_len) {
        string out;
        out.reserve(fields * (value_len + 16));
        for (size_t i = 0; i < fields; ++i) {
            out += "name";
            append_index(out, i);
            out += ": ";
            const char fill = static_cast<char>('a' + static_cast<int>(i % 26));
            for (size_t k = 0; k < value_len; ++k) {
                out += fill;
            }
            out += '\n';
        }
        return out;
    }

    string make_sequence_of_mappings(const size_t items) {
        string out;
        out.reserve(items * 80);
        for (size_t i = 0; i < items; ++i) {
            out += "- id: ";
            append_index(out, i);
            out += "\n  name: item";
            append_index(out, i);
            out += "\n  value: ";
            append_index(out, i);
            out += ".5\n  active: ";
            out += i % 2 == 0 ? "true" : "false";
            out += '\n';
        }
        return out;
    }

    string make_nested_mappings(const size_t depth, const size_t leaves) {
        string out;
        for (size_t d = 0; d < depth; ++d) {
            for (size_t i = 0; i < d * 2; ++i) {
                out += ' ';
            }
            out += "level";
            append_index(out, d);
            out += ":\n";
        }
        for (size_t i = 0; i < leaves; ++i) {
            for (size_t k = 0; k < depth * 2; ++k) {
                out += ' ';
            }
            out += "leaf";
            append_index(out, i);
            out += ": ";
            append_index(out, i);
            out += '\n';
        }
        return out;
    }

    string make_flow_mapping(const size_t fields) {
        string out;
        out.reserve(fields * 16);
        out += '{';
        for (size_t i = 0; i < fields; ++i) {
            if (i != 0) {
                out += ", ";
            }
            out += "key";
            append_index(out, i);
            out += ": ";
            append_index(out, i * 3);
        }
        out += "}\n";
        return out;
    }

    string make_block_sequence(const size_t items) {
        string out;
        out.reserve(items * 8);
        for (size_t i = 0; i < items; ++i) {
            out += "- ";
            append_index(out, i);
            out += '\n';
        }
        return out;
    }

    string make_flow_sequence(const size_t items) {
        string out;
        out.reserve(items * 8);
        out += '[';
        for (size_t i = 0; i < items; ++i) {
            if (i != 0) {
                out += ", ";
            }
            append_index(out, i);
        }
        out += "]\n";
        return out;
    }

    struct document {
        const char* name = "";
        string text;
    };

    vector<document> documents() {
        vector<document> docs;
        docs.push_back(document{"FlatMapping_32", make_flat_mapping(32)});
        docs.push_back(document{"FlatMapping_256", make_flat_mapping(256)});
        docs.push_back(document{"StringMapping_64x32", make_string_mapping(64, 32)});
        docs.push_back(document{"SequenceOfMappings_256", make_sequence_of_mappings(256)});
        docs.push_back(document{"SequenceOfMappings_2048", make_sequence_of_mappings(2048)});
        docs.push_back(document{"NestedMappings_8x32", make_nested_mappings(8, 32)});
        docs.push_back(document{"FlowMapping_256", make_flow_mapping(256)});
        docs.push_back(document{"BlockSequence_2048", make_block_sequence(2048)});
        docs.push_back(document{"FlowSequence_2048", make_flow_sequence(2048)});
        return docs;
    }
} // namespace

static void BM_YamlParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        yaml_parser parser(doc.text);
        yaml_document root = parser.parse();
        benchmark::DoNotOptimize(root.root());
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_YamlParseBorrow(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        yaml_parser parser(string_view(doc.text.data(), doc.text.size()));
        yaml_document root = parser.parse();
        benchmark::DoNotOptimize(root.root());
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_YamlTryParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    for (auto _: state) {
        yaml_parser parser(doc.text);
        auto root = parser.try_parse();
        if (!root.has_value()) {
            state.SkipWithError("parse failed");
            return;
        }
        benchmark::DoNotOptimize(root.value().root());
    }
    state.SetLabel(doc.name);
}

static void BM_YamlSerialize(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    yaml_parser parser(doc.text);
    yaml_document root = parser.parse();
    for (auto _: state) {
        string out = root->to_document();
        benchmark::DoNotOptimize(out.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_YamlBuild(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        yaml_builder builder;
        for (size_t i = 0; i < fields; ++i) {
            string key = "key";
            append_index(key, i);
            builder.key(key).value(static_cast<int64_t>(i * 3));
        }
        yaml_document root = builder.build();
        benchmark::DoNotOptimize(root.root());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

static void BM_YamlBuildSequence(benchmark::State& state) {
    const auto items = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        yaml_builder builder;
        builder.key("items").begin_sequence();
        for (size_t i = 0; i < items; ++i) {
            builder.value(static_cast<int64_t>(i));
        }
        builder.end_sequence();
        yaml_document root = builder.build();
        benchmark::DoNotOptimize(root.root());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(items));
}

static void BM_YamlLookup(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    const string text = make_flat_mapping(fields);
    yaml_parser parser(string_view(text.data(), text.size()));
    yaml_document root = parser.parse();
    const yaml_mapping mapping = root->as_mapping();
    string probe = "key";
    append_index(probe, fields / 2);
    for (auto _: state) {
        const yaml_value* value = mapping.get_member(probe);
        benchmark::DoNotOptimize(value);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_YamlStringExtract(benchmark::State& state) {
    const auto fields = static_cast<size_t>(state.range(0));
    const string text = make_string_mapping(fields, 32);
    yaml_parser parser(string_view(text.data(), text.size()));
    yaml_document root = parser.parse();
    const yaml_mapping mapping = root->as_mapping();
    for (auto _: state) {
        size_t total = 0;
        for (const yaml_member& member: mapping) {
            total += member.value.as_string().get_value().size();
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(fields));
}

BENCHMARK(BM_YamlParse)->DenseRange(0, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_YamlParseBorrow)->DenseRange(0, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_YamlTryParse)->DenseRange(0, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_YamlSerialize)->DenseRange(0, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_YamlBuild)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_YamlBuildSequence)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_YamlLookup)->Arg(32)->Arg(256)->Arg(2048)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_YamlStringExtract)->Arg(64)->Arg(512)->Arg(2048)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();
