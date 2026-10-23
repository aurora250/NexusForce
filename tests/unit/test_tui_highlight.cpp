#include <NeForce/tui/dom/highlight.hpp>
#include <gtest/gtest.h>

using namespace neforce;
using namespace neforce::tui;

namespace {
    syntax_class class_at(const vector<syntax_run>& runs, const size_t offset) {
        for (size_t i = 0; i < runs.size(); ++i) {
            if (offset >= runs[i].begin && offset < runs[i].end) {
                return runs[i].cls;
            }
        }
        return syntax_class::plain;
    }

    size_t offset_of(const char* text, const char* needle) {
        const string_view haystack(text);
        const size_t found = haystack.find(string_view(needle));
        return found == string_view::npos ? 0 : found;
    }

    const highlighter* lookup(const char* name) { return find_highlighter(string_view(name)); }

    vector<syntax_run> highlight(const char* language, const char* text, highlight_state& state) {
        vector<syntax_run> out;
        const highlighter* h = lookup(language);
        if (h != nullptr) {
            h->highlight_line(string_view(text), state, out);
        }
        return out;
    }

    vector<syntax_run> highlight(const char* language, const char* text) {
        highlight_state state;
        return highlight(language, text, state);
    }

    bool ordered(const vector<syntax_run>& runs, const size_t length) {
        size_t previous_end = 0;
        for (size_t i = 0; i < runs.size(); ++i) {
            if (runs[i].begin >= runs[i].end || runs[i].end > length || runs[i].begin < previous_end) {
                return false;
            }
            previous_end = runs[i].end;
        }
        return true;
    }

    const char* const canonical_names[] = {"cpp",      "c",    "rust", "go",     "python", "javascript", "typescript",
                                           "json",     "yaml", "toml", "bash",   "sql",    "html",       "css",
                                           "markdown", "diff", "java", "kotlin", "swift",  "lua"};

    struct alias_case {
        const char* alias;
        const char* canonical;
    };

    const alias_case alias_cases[] = {
            {"c++", "cpp"},
            {"cc", "cpp"},
            {"cxx", "cpp"},
            {"h", "cpp"},
            {"hpp", "cpp"},
            {"py", "python"},
            {"js", "javascript"},
            {"jsx", "javascript"},
            {"mjs", "javascript"},
            {"ts", "typescript"},
            {"tsx", "typescript"},
            {"yml", "yaml"},
            {"sh", "bash"},
            {"shell", "bash"},
            {"zsh", "bash"},
            {"xml", "html"},
            {"svg", "html"},
            {"md", "markdown"},
            {"patch", "diff"},
            {"kt", "kotlin"},
            {"CPP", "cpp"},
            {"JavaScript", "javascript"},
            {"  .TS  ", "typescript"},
    };
} // namespace

TEST(TuiHighlightRegistryTest, ReportsEverySupportedLanguage) {
    ASSERT_EQ(highlighter_count(), static_cast<size_t>(20));
    for (size_t i = 0; i < highlighter_count(); ++i) {
        EXPECT_TRUE(highlighter_language(i).equal_to(canonical_names[i]));
        EXPECT_NE(lookup(canonical_names[i]), nullptr);
    }
}

TEST(TuiHighlightRegistryTest, ResolvesAliasesCaseInsensitively) {
    for (const alias_case& item: alias_cases) {
        SCOPED_TRACE(item.alias);
        EXPECT_EQ(lookup(item.alias), lookup(item.canonical));
    }
}

TEST(TuiHighlightRegistryTest, RejectsUnknownLanguages) {
    EXPECT_EQ(lookup("brainfuck"), nullptr);
    EXPECT_EQ(lookup("rust-lang"), nullptr);
    EXPECT_EQ(lookup("unknown"), nullptr);
    EXPECT_EQ(lookup(""), nullptr);
    EXPECT_EQ(find_highlighter(string_view()), nullptr);
}

TEST(TuiHighlightRegistryTest, RejectsOutOfRangeIndices) {
    EXPECT_TRUE(highlighter_language(highlighter_count()).empty());
    EXPECT_TRUE(highlighter_language(highlighter_count() + 100).empty());
    EXPECT_FALSE(highlighter_language(0).empty());
}

TEST(TuiHighlightTest, ReturnsNoRunsForEmptyLines) {
    for (const char* name: canonical_names) {
        highlight_state state;
        const vector<syntax_run> runs = highlight(name, "", state);
        EXPECT_TRUE(runs.empty());
        EXPECT_EQ(state.current, highlight_state::mode::normal);
        EXPECT_EQ(state.quote, 0);
        EXPECT_FALSE(state.escaped);
    }
}

TEST(TuiHighlightTest, ClearsOutputOnEveryCall) {
    const highlighter* h = lookup("cpp");
    ASSERT_NE(h, nullptr);
    highlight_state state;
    vector<syntax_run> out;
    h->highlight_line(string_view("int x"), state, out);
    ASSERT_FALSE(out.empty());
    h->highlight_line(string_view("plain words"), state, out);
    EXPECT_TRUE(out.empty());
}

TEST(TuiHighlightCppTest, ClassifiesKeywordsTypesVariablesAndNumbers) {
    const char* line = "const int value = 42; // note";
    const vector<syntax_run> runs = highlight("cpp", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "const")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "int")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "value")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "42")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "// note")), syntax_class::comment);
    EXPECT_EQ(class_at(runs, offset_of(line, "=")), syntax_class::op);
    EXPECT_EQ(class_at(runs, offset_of(line, ";")), syntax_class::punctuation);
}

TEST(TuiHighlightCppTest, DetectsCallsAndLeavesKeywordsAlone) {
    const char* line = "int main() { if (ready) { return compute(1); } }";
    const vector<syntax_run> runs = highlight("cpp", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "int")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "main")), syntax_class::function);
    EXPECT_EQ(class_at(runs, offset_of(line, "if")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "return")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "compute")), syntax_class::function);
    EXPECT_EQ(class_at(runs, offset_of(line, "(")), syntax_class::punctuation);
    EXPECT_EQ(class_at(runs, offset_of(line, "1")), syntax_class::number);
}

TEST(TuiHighlightCppTest, HandlesEscapedStringsAndCharLiterals) {
    const char* line = "const char* s = \"a\\\"b\"; char c = 'x';";
    const vector<syntax_run> runs = highlight("cpp", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "\"a")), syntax_class::string);
    EXPECT_EQ(class_at(runs, offset_of(line, "\\\"b")), syntax_class::string);
    EXPECT_EQ(class_at(runs, offset_of(line, "'x'")), syntax_class::string);
    EXPECT_EQ(class_at(runs, offset_of(line, "const")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "char")), syntax_class::type);
}

TEST(TuiHighlightCppTest, RecognizesNumericForms) {
    const char* line = "int n = 0xFF + 0b1010 + 1_000 + 3.14e-2;";
    const vector<syntax_run> runs = highlight("cpp", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "0xFF")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "0xFF") + 3), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "0b1010")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "1_000")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "3.14e-2")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "3.14e-2") + 5), syntax_class::number);
}

TEST(TuiHighlightCppTest, PersistsBlockCommentsAcrossLines) {
    highlight_state state;
    const vector<syntax_run> first = highlight("cpp", "int x; /* start", state);
    EXPECT_EQ(state.current, highlight_state::mode::block_comment);
    EXPECT_EQ(class_at(first, offset_of("int x; /* start", "/*")), syntax_class::comment);
    const char* second = "still */ int y = 1;";
    const vector<syntax_run> after = highlight("cpp", second, state);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(state.quote, 0);
    EXPECT_EQ(class_at(after, 0), syntax_class::comment);
    EXPECT_EQ(class_at(after, offset_of(second, "int")), syntax_class::type);
    EXPECT_EQ(class_at(after, offset_of(second, "y")), syntax_class::variable);
    EXPECT_EQ(class_at(after, offset_of(second, "1")), syntax_class::number);
}

TEST(TuiHighlightCppTest, ClosesSingleLineBlockComments) {
    highlight_state state;
    const char* line = "int a; /* note */ int b;";
    const vector<syntax_run> runs = highlight("cpp", line, state);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(class_at(runs, offset_of(line, "/*")), syntax_class::comment);
    EXPECT_EQ(class_at(runs, offset_of(line, "note")), syntax_class::comment);
    EXPECT_EQ(class_at(runs, offset_of(line, "int b")), syntax_class::type);
}

TEST(TuiHighlightCppTest, RecognizesPreprocessorDirectives) {
    const char* line = "  #include <cstdio>";
    const vector<syntax_run> runs = highlight("cpp", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "#include")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "<")), syntax_class::op);
}

TEST(TuiHighlightCppTest, EmitsNothingForPlainText) {
    const vector<syntax_run> runs = highlight("cpp", "hello world");
    EXPECT_TRUE(runs.empty());
}

TEST(TuiHighlightCTest, ClassifiesCoreTokens) {
    const char* line = "static int count = 0; // note";
    const vector<syntax_run> runs = highlight("c", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "static")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "int")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "count")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "0")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "// note")), syntax_class::comment);
}

TEST(TuiHighlightRustTest, ClassifiesDeclarationsAndLifetimes) {
    const char* line = "fn main() { let x: i32 = 1; }";
    const vector<syntax_run> runs = highlight("rust", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "fn")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "main")), syntax_class::function);
    EXPECT_EQ(class_at(runs, offset_of(line, "let")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "x:")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "i32")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "1")), syntax_class::number);
    const char* lifetime = "&'a str";
    const vector<syntax_run> lifetime_runs = highlight("rust", lifetime);
    EXPECT_EQ(class_at(lifetime_runs, offset_of(lifetime, "'a")), syntax_class::variable);
    EXPECT_EQ(class_at(lifetime_runs, offset_of(lifetime, "str")), syntax_class::type);
    EXPECT_EQ(class_at(highlight("rust", "// note"), 0), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("rust", "true"), 0), syntax_class::constant);
}

TEST(TuiHighlightGoTest, ClassifiesDeclarationsAndRawStrings) {
    highlight_state state;
    const char* line = "func main() { var s string = nil }";
    const vector<syntax_run> runs = highlight("go", line, state);
    EXPECT_EQ(class_at(runs, offset_of(line, "func")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "main")), syntax_class::function);
    EXPECT_EQ(class_at(runs, offset_of(line, "var")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "string")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "nil")), syntax_class::constant);
    const vector<syntax_run> raw = highlight("go", "s := `raw", state);
    EXPECT_EQ(state.current, highlight_state::mode::string_literal);
    EXPECT_EQ(state.quote, '`');
    EXPECT_EQ(class_at(raw, offset_of("s := `raw", "`raw")), syntax_class::string);
    const vector<syntax_run> tail = highlight("go", "text`", state);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(class_at(tail, 0), syntax_class::string);
}

TEST(TuiHighlightPythonTest, ClassifiesDefinitionsConstantsAndComments) {
    const char* line = "def greet(name): # note";
    const vector<syntax_run> runs = highlight("python", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "def")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "greet")), syntax_class::function);
    EXPECT_EQ(class_at(runs, offset_of(line, "# note")), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("python", "None"), 0), syntax_class::constant);
    EXPECT_EQ(class_at(highlight("python", "True"), 0), syntax_class::constant);
    EXPECT_EQ(class_at(highlight("python", "str"), 0), syntax_class::type);
    EXPECT_EQ(class_at(highlight("python", "if (ready):"), 0), syntax_class::keyword);
    const char* annotated = "value: int = 1";
    const vector<syntax_run> annotated_runs = highlight("python", annotated);
    EXPECT_EQ(class_at(annotated_runs, offset_of(annotated, "value")), syntax_class::variable);
    EXPECT_EQ(class_at(annotated_runs, offset_of(annotated, "int")), syntax_class::type);
    EXPECT_EQ(class_at(annotated_runs, offset_of(annotated, "1")), syntax_class::number);
}

TEST(TuiHighlightPythonTest, PersistsTripleQuotedStrings) {
    highlight_state state;
    const char* first = "s = \"\"\"hello";
    const vector<syntax_run> opening = highlight("python", first, state);
    EXPECT_EQ(state.current, highlight_state::mode::string_literal);
    EXPECT_EQ(state.quote, '"');
    EXPECT_EQ(class_at(opening, offset_of(first, "\"\"\"")), syntax_class::string);
    const char* second = "world\"\"\"";
    const vector<syntax_run> closing = highlight("python", second, state);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(class_at(closing, 0), syntax_class::string);
    EXPECT_EQ(class_at(closing, offset_of(second, "\"\"\"")), syntax_class::string);
}

TEST(TuiHighlightPythonTest, HonorsTrailingEscapesInMultilineStrings) {
    highlight_state state;
    const vector<syntax_run> opening = highlight("python", "s = \"\"\"hello\\", state);
    EXPECT_EQ(state.current, highlight_state::mode::string_literal);
    EXPECT_TRUE(state.escaped);
    EXPECT_EQ(class_at(opening, offset_of("s = \"\"\"hello\\", "hello")), syntax_class::string);
    const char* second = "world\"\"\"";
    const vector<syntax_run> closing = highlight("python", second, state);
    EXPECT_FALSE(state.escaped);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(class_at(closing, 0), syntax_class::string);
}

TEST(TuiHighlightJavaScriptTest, ClassifiesCoreTokens) {
    const char* line = "const value = null;";
    const vector<syntax_run> runs = highlight("javascript", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "const")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "value")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "null")), syntax_class::constant);
    const char* call = "function run() { return new Promise(); }";
    const vector<syntax_run> call_runs = highlight("javascript", call);
    EXPECT_EQ(class_at(call_runs, offset_of(call, "function")), syntax_class::keyword);
    EXPECT_EQ(class_at(call_runs, offset_of(call, "run")), syntax_class::function);
    EXPECT_EQ(class_at(call_runs, offset_of(call, "return")), syntax_class::keyword);
    EXPECT_EQ(class_at(call_runs, offset_of(call, "new")), syntax_class::keyword);
    EXPECT_EQ(class_at(call_runs, offset_of(call, "Promise")), syntax_class::type);
    const char* arrow = "const f = (x) => x;";
    const vector<syntax_run> arrow_runs = highlight("javascript", arrow);
    EXPECT_EQ(class_at(arrow_runs, offset_of(arrow, "=>")), syntax_class::op);
    EXPECT_EQ(class_at(arrow_runs, offset_of(arrow, ";")), syntax_class::punctuation);
    EXPECT_EQ(class_at(highlight("javascript", "// note"), 0), syntax_class::comment);
}

TEST(TuiHighlightJavaScriptTest, PersistsTemplateStrings) {
    highlight_state state;
    const char* first = "const s = `hello";
    const vector<syntax_run> opening = highlight("javascript", first, state);
    EXPECT_EQ(state.current, highlight_state::mode::string_literal);
    EXPECT_EQ(state.quote, '`');
    EXPECT_EQ(class_at(opening, offset_of(first, "`hello")), syntax_class::string);
    const char* second = "world`;";
    const vector<syntax_run> closing = highlight("javascript", second, state);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(class_at(closing, 0), syntax_class::string);
    EXPECT_EQ(class_at(closing, offset_of(second, ";")), syntax_class::punctuation);
}

TEST(TuiHighlightTypeScriptTest, ClassifiesTypesAndKeywords) {
    const char* line = "const count: number = 0;";
    const vector<syntax_run> runs = highlight("typescript", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "const")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "number")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "0")), syntax_class::number);
    EXPECT_EQ(class_at(highlight("typescript", "interface User {"), 0), syntax_class::keyword);
    EXPECT_EQ(class_at(highlight("typescript", "readonly"), 0), syntax_class::keyword);
}

TEST(TuiHighlightJsonTest, ClassifiesObjectsAndConstants) {
    const char* line = "{\"name\": \"neo\", \"age\": 42, \"ok\": false}";
    const vector<syntax_run> runs = highlight("json", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "\"name\"")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "\"neo\"")), syntax_class::string);
    EXPECT_EQ(class_at(runs, offset_of(line, "\"age\"")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "42")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "false")), syntax_class::constant);
    EXPECT_EQ(class_at(runs, offset_of(line, ":")), syntax_class::punctuation);
    const char* escaped = "{\"note\": \"a\\\"b\"}";
    const vector<syntax_run> escaped_runs = highlight("json", escaped);
    EXPECT_EQ(class_at(escaped_runs, offset_of(escaped, "\"a")), syntax_class::string);
    EXPECT_EQ(class_at(highlight("json", "'x'"), 0), syntax_class::plain);
}

TEST(TuiHighlightYamlTest, ClassifiesKeysConstantsAndComments) {
    const char* line = "name: neo # note";
    const vector<syntax_run> runs = highlight("yaml", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "name")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "# note")), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("yaml", "enabled: true"), offset_of("enabled: true", "true")), syntax_class::constant);
    EXPECT_EQ(class_at(highlight("yaml", "version: 1.2"), offset_of("version: 1.2", "1.2")), syntax_class::number);
    EXPECT_EQ(class_at(highlight("yaml", "echo a#b"), offset_of("echo a#b", "#")), syntax_class::punctuation);
}

TEST(TuiHighlightTomlTest, ClassifiesKeysStringsAndConstants) {
    const char* line = "port = 8080";
    const vector<syntax_run> runs = highlight("toml", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "port")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "8080")), syntax_class::number);
    EXPECT_EQ(class_at(highlight("toml", "name = \"neo\""), offset_of("name = \"neo\"", "\"neo\"")),
              syntax_class::string);
    EXPECT_EQ(class_at(highlight("toml", "enabled = true"), offset_of("enabled = true", "true")),
              syntax_class::constant);
    EXPECT_EQ(class_at(highlight("toml", "# note"), 0), syntax_class::comment);
}

TEST(TuiHighlightBashTest, ClassifiesKeywordsVariablesAndComments) {
    const char* line = "if [ -n \"$HOME\" ]; then echo $USER; fi # note";
    const vector<syntax_run> runs = highlight("bash", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "if")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "then")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "fi")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "\"$HOME\"")), syntax_class::string);
    EXPECT_EQ(class_at(runs, offset_of(line, "$USER")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "# note")), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("bash", "echo a#b"), offset_of("echo a#b", "#")), syntax_class::punctuation);
    EXPECT_EQ(class_at(highlight("bash", "value=\"text\""), 0), syntax_class::variable);
}

TEST(TuiHighlightSqlTest, IsCaseInsensitiveAndSupportsQuotes) {
    const char* line = "SELECT id FROM users WHERE active = true;";
    const vector<syntax_run> runs = highlight("sql", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "SELECT")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "FROM")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "WHERE")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "active")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "true")), syntax_class::constant);
    const char* typed = "CREATE TABLE t (id INT);";
    const vector<syntax_run> typed_runs = highlight("sql", typed);
    EXPECT_EQ(class_at(typed_runs, offset_of(typed, "CREATE")), syntax_class::keyword);
    EXPECT_EQ(class_at(typed_runs, offset_of(typed, "INT")), syntax_class::type);
    const char* quoted = "INSERT INTO t VALUES ('it''s');";
    const vector<syntax_run> quoted_runs = highlight("sql", quoted);
    EXPECT_EQ(class_at(quoted_runs, offset_of(quoted, "'it''s'")), syntax_class::string);
    EXPECT_EQ(class_at(quoted_runs, offset_of(quoted, "'it''s'") + 3), syntax_class::string);
    EXPECT_EQ(class_at(highlight("sql", "-- note"), 0), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("sql", "/* note */"), 0), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("sql", "SELECT NULL FROM t"), offset_of("SELECT NULL FROM t", "NULL")),
              syntax_class::constant);
}

TEST(TuiHighlightHtmlTest, ClassifiesTagsAttributesAndComments) {
    const char* line = "<div class=\"box\">table</div>";
    const vector<syntax_run> runs = highlight("html", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "div")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "class")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "\"box\"")), syntax_class::string);
    EXPECT_EQ(class_at(runs, offset_of(line, "table")), syntax_class::plain);
    EXPECT_EQ(class_at(highlight("html", "<custom-element data-id=\"7\">"),
                       offset_of("<custom-element data-id=\"7\">", "custom-element")),
              syntax_class::keyword);
    EXPECT_EQ(class_at(highlight("html", "<custom-element data-id=\"7\">"),
                       offset_of("<custom-element data-id=\"7\">", "data-id")),
              syntax_class::variable);
}

TEST(TuiHighlightHtmlTest, PersistsMarkupComments) {
    highlight_state state;
    const vector<syntax_run> first = highlight("html", "<!-- start", state);
    EXPECT_EQ(state.current, highlight_state::mode::block_comment);
    EXPECT_EQ(class_at(first, 0), syntax_class::comment);
    const char* second = "end --> <b>hi</b>";
    const vector<syntax_run> after = highlight("html", second, state);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(class_at(after, 0), syntax_class::comment);
    EXPECT_EQ(class_at(after, offset_of(second, "b>")), syntax_class::keyword);
    EXPECT_EQ(class_at(after, offset_of(second, "hi")), syntax_class::plain);
}

TEST(TuiHighlightCssTest, ClassifiesPropertiesUnitsAndColors) {
    const char* line = "font-size: 10px; color: #fff;";
    const vector<syntax_run> runs = highlight("css", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "font-size")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "10px")), syntax_class::number);
    EXPECT_EQ(class_at(runs, offset_of(line, "px")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "color")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "#fff")), syntax_class::number);
    EXPECT_EQ(class_at(highlight("css", "@media screen {"), offset_of("@media screen {", "media")),
              syntax_class::keyword);
    EXPECT_EQ(class_at(highlight("css", "/* note */"), 0), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("css", "margin: -5px;"), offset_of("margin: -5px;", "5px")), syntax_class::number);
}


TEST(TuiHighlightJavaTest, ClassifiesModifiersTypesAndStrings) {
    const char* line = "public class Foo { String name = \"neo\"; }";
    const vector<syntax_run> runs = highlight("java", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "public")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "class")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "String")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "name")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "\"neo\"")), syntax_class::string);
    EXPECT_EQ(class_at(highlight("java", "// note"), 0), syntax_class::comment);
}

TEST(TuiHighlightKotlinTest, ClassifiesTypeAnnotations) {
    const char* line = "val count: Int = 1";
    const vector<syntax_run> runs = highlight("kotlin", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "val")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "count")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "Int")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "1")), syntax_class::number);
    EXPECT_EQ(class_at(highlight("kotlin", "fun main() {}"), offset_of("fun main() {}", "main")),
              syntax_class::function);
}

TEST(TuiHighlightSwiftTest, ClassifiesTypeAnnotations) {
    const char* line = "let count: Int = 1";
    const vector<syntax_run> runs = highlight("swift", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "let")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "count")), syntax_class::variable);
    EXPECT_EQ(class_at(runs, offset_of(line, "Int")), syntax_class::type);
    EXPECT_EQ(class_at(runs, offset_of(line, "1")), syntax_class::number);
    EXPECT_EQ(class_at(highlight("swift", "nil"), 0), syntax_class::constant);
}

TEST(TuiHighlightLuaTest, ClassifiesFunctionsAndLongComments) {
    const char* line = "local function foo() end";
    const vector<syntax_run> runs = highlight("lua", line);
    EXPECT_EQ(class_at(runs, offset_of(line, "local")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "function")), syntax_class::keyword);
    EXPECT_EQ(class_at(runs, offset_of(line, "foo")), syntax_class::function);
    EXPECT_EQ(class_at(runs, offset_of(line, "end")), syntax_class::keyword);
    highlight_state state;
    const vector<syntax_run> opening = highlight("lua", "--[[ start", state);
    EXPECT_EQ(state.current, highlight_state::mode::block_comment);
    EXPECT_EQ(class_at(opening, 0), syntax_class::comment);
    const vector<syntax_run> closing = highlight("lua", "end ]] local x = 1", state);
    EXPECT_EQ(state.current, highlight_state::mode::normal);
    EXPECT_EQ(class_at(closing, offset_of("end ]] local x = 1", "local")), syntax_class::keyword);
    EXPECT_EQ(class_at(closing, offset_of("end ]] local x = 1", "x")), syntax_class::variable);
}

TEST(TuiHighlightMarkdownTest, ClassifiesStructure) {
    const char* heading = "## Title";
    const vector<syntax_run> heading_runs = highlight("markdown", heading);
    EXPECT_EQ(class_at(heading_runs, offset_of(heading, "##")), syntax_class::keyword);
    EXPECT_EQ(class_at(heading_runs, offset_of(heading, "Title")), syntax_class::type);
    const char* marker = "- item";
    const vector<syntax_run> marker_runs = highlight("markdown", marker);
    EXPECT_EQ(class_at(marker_runs, 0), syntax_class::keyword);
    const char* ordered_marker = "1. item";
    EXPECT_EQ(class_at(highlight("markdown", ordered_marker), 0), syntax_class::keyword);
    EXPECT_EQ(class_at(highlight("markdown", "```cpp"), 0), syntax_class::keyword);
    EXPECT_EQ(class_at(highlight("markdown", "~~~"), 0), syntax_class::keyword);
    const char* inline_code = "some `code` here";
    EXPECT_EQ(class_at(highlight("markdown", inline_code), offset_of(inline_code, "`code`")), syntax_class::string);
    EXPECT_EQ(class_at(highlight("markdown", "**bold**"), 0), syntax_class::op);
    EXPECT_EQ(class_at(highlight("markdown", "> quote"), 0), syntax_class::punctuation);
}

TEST(TuiHighlightDiffTest, ClassifiesPatchLines) {
    const char* added = "+added line";
    EXPECT_EQ(class_at(highlight("diff", added), 0), syntax_class::string);
    EXPECT_EQ(class_at(highlight("diff", added), offset_of(added, "line")), syntax_class::string);
    EXPECT_EQ(class_at(highlight("diff", "-removed line"), 0), syntax_class::comment);
    EXPECT_EQ(class_at(highlight("diff", "@@ -1,2 +3,4 @@"), 0), syntax_class::keyword);
    EXPECT_TRUE(highlight("diff", " context line").empty());
}

TEST(TuiHighlightLanguagesTest, ClassifiesKeywordPerLanguage) {
    struct sample {
        const char* language;
        const char* text;
        const char* token;
    };
    const sample samples[] = {
            {"cpp", "class Foo", "class"},
            {"c", "static int x", "static"},
            {"rust", "fn main", "fn"},
            {"go", "func main", "func"},
            {"python", "def main", "def"},
            {"javascript", "const x", "const"},
            {"typescript", "interface X", "interface"},
            {"json", "true", "true"},
            {"yaml", "enabled: true", "true"},
            {"toml", "enabled = true", "true"},
            {"bash", "if true", "if"},
            {"sql", "SELECT 1", "SELECT"},
            {"html", "<div>", "div"},
            {"css", "@media", "media"},
            {"java", "public class X", "public"},
            {"kotlin", "fun main", "fun"},
            {"swift", "func main", "func"},
            {"lua", "local x", "local"},
    };
    for (const sample& item: samples) {
        SCOPED_TRACE(item.language);
        const vector<syntax_run> runs = highlight(item.language, item.text);
        EXPECT_NE(class_at(runs, offset_of(item.text, item.token)), syntax_class::plain);
    }
}

TEST(TuiHighlightLanguagesTest, KeepsRunsOrderedAndInsideTheLine) {
    const char* line = "foo = bar(1); // \"str\" 0xFF @@@ ***";
    for (const char* name: canonical_names) {
        SCOPED_TRACE(name);
        const vector<syntax_run> runs = highlight(name, line);
        EXPECT_TRUE(ordered(runs, string_view(line).size()));
    }
}

TEST(TuiHighlightLanguagesTest, KeepsStateNormalAfterSingleLineInputs) {
    for (const char* name: canonical_names) {
        SCOPED_TRACE(name);
        highlight_state state;
        highlight(name, "plain text 123", state);
        EXPECT_EQ(state.current, highlight_state::mode::normal);
    }
}
