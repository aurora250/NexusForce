#include <benchmark/benchmark.h>
#include <NeForce/core/file/md/markdown_parser.hpp>
#include <NeForce/tui/dom/markdown.hpp>
using namespace neforce;
using namespace neforce::tui;

namespace {
    string make_document(const size_t blocks) {
        string text;
        for (size_t i = 0; i < blocks; ++i) {
            text += "## Section ";
            text += to_string(static_cast<int64_t>(i));
            text += "\n\nSome **bold** and `code` in a paragraph with a [link](https://example.com).\n\n";
            text += "- first item\n- second item with more words\n\n";
            text += "```cpp\nint value = 42;\n```\n\n";
        }
        return text;
    }
} // namespace

static void BmMarkdownParse(benchmark::State& state) {
    const string source = make_document(static_cast<size_t>(state.range(0)));
    markdown_option opt;
    opt.width = 100;
    for (auto _: state) {
        const element tree = markdown(source.view(), opt);
        benchmark::DoNotOptimize(tree.children().size());
    }
}

static void BmMarkdownStreamAppend(benchmark::State& state) {
    const string source = make_document(static_cast<size_t>(state.range(0)));
    markdown_option opt;
    opt.width = 100;
    for (auto _: state) {
        markdown_stream stream(opt);
        for (size_t i = 0; i < source.size(); i += 512) {
            stream.append(source.view().substr(i, min<size_t>(512, source.size() - i)));
            const element& view = stream.render(100);
            benchmark::DoNotOptimize(view.children().size());
        }
    }
}

// Parser only: the delta against BmMarkdownParse is what the element tree costs.
static void BmMarkdownParserOnly(benchmark::State& state) {
    const string source = make_document(static_cast<size_t>(state.range(0)));
    for (auto _: state) {
        const auto document = markdown_parser(string(source)).parse();
        benchmark::DoNotOptimize(document.blocks.size());
    }
}

BENCHMARK(BmMarkdownParse)->Range(1, 32);
BENCHMARK(BmMarkdownParserOnly)->Range(1, 32);
BENCHMARK(BmMarkdownStreamAppend)->Range(1, 32);

BENCHMARK_MAIN();
