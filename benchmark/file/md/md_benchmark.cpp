#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/md/markdown_builder.hpp>
#include <NeForce/core/file/md/markdown_parser.hpp>

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

    string make_prose(const size_t paragraphs) {
        string out;
        out.reserve(paragraphs * 220);
        for (size_t i = 0; i < paragraphs; ++i) {
            out += "Paragraph ";
            append_index(out, i);
            out += " has **strong text** and *emphasis*, a [link](https://example.com/";
            append_index(out, i);
            out += ") plus `inline code` and ~~deleted~~ words. It also contains a sentence with several words to make "
                   "the scan realistic, including punctuation, numbers 12345, and UTF-8 text: 你好世界。\n\n";
        }
        return out;
    }

    string make_lists(const size_t items) {
        string out;
        out.reserve(items * 80);
        for (size_t i = 0; i < items; ++i) {
            out += "- item ";
            append_index(out, i);
            out += "\n  - nested ";
            append_index(out, i);
            out += "\n    continuation text\n";
        }
        return out;
    }

    string make_table(const size_t rows) {
        string out;
        out.reserve(rows * 64);
        out += "| name | value | note | flag |\n| :-- | --: | :-: | --- |\n";
        for (size_t i = 0; i < rows; ++i) {
            out += "| row";
            append_index(out, i);
            out += " | ";
            append_index(out, i * 7);
            out += " | note ";
            append_index(out, i);
            out += " | yes |\n";
        }
        return out;
    }

    string make_code(const size_t blocks) {
        string out;
        out.reserve(blocks * 160);
        for (size_t i = 0; i < blocks; ++i) {
            out += "```cpp\nauto value";
            append_index(out, i);
            out += " = compute(";
            append_index(out, i);
            out += ");\nreturn value";
            append_index(out, i);
            out += ";\n```\n\n";
        }
        return out;
    }

    string make_mixed(const size_t sections) {
        string out;
        out.reserve(sections * 320);
        for (size_t i = 0; i < sections; ++i) {
            out += "## Section ";
            append_index(out, i);
            out += "\n\n> quoted paragraph with **bold** and `code`\n\n- first item\n- second item with "
                   "[link](https://example.com)\n\n```\nfenced ";
            append_index(out, i);
            out += "\n```\n\n";
        }
        return out;
    }

    struct document {
        const char* name = "";
        string text;
    };

    vector<document> documents() {
        vector<document> docs;
        docs.push_back(document{"Prose_512", make_prose(512)});
        docs.push_back(document{"Lists_512", make_lists(512)});
        docs.push_back(document{"Table_256", make_table(256)});
        docs.push_back(document{"CodeFence_256", make_code(256)});
        docs.push_back(document{"Mixed_512", make_mixed(512)});
        return docs;
    }
} // namespace

static void BM_MdParse(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    size_t parsed = 0;
    for (auto _: state) {
        markdown_parser parser(doc.text);
        markdown_document document = parser.parse();
        benchmark::DoNotOptimize(document.blocks);
        ++parsed;
    }
    state.SetBytesProcessed(static_cast<int64_t>(parsed) * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_MdSerialize(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    markdown_parser parser(doc.text);
    markdown_document document = parser.parse();
    for (auto _: state) {
        string out = markdown_builder::dump(document);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

static void BM_MdParseInline(benchmark::State& state) {
    const auto docs = documents();
    const auto& doc = docs[static_cast<size_t>(state.range(0))];
    for (auto _: state) {
        markdown_parser parser(doc.text);
        md_inlines inlines = parser.parse_inline();
        benchmark::DoNotOptimize(inlines);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(doc.text.size()));
    state.SetLabel(doc.name);
}

BENCHMARK(BM_MdParse)->DenseRange(0, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_MdSerialize)->DenseRange(0, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_MdParseInline)->DenseRange(0, 4)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();
