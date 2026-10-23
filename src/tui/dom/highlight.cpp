#include <NeForce/core/string/char_types.hpp>
#include <NeForce/tui/dom/highlight.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    constexpr unsigned flag_backtick = 1U << 0;
    constexpr unsigned flag_triple_single = 1U << 1;
    constexpr unsigned flag_triple_double = 1U << 2;
    constexpr unsigned flag_backtick_raw = 1U << 3;
    constexpr unsigned flag_doubled_quote = 1U << 4;
    constexpr unsigned flag_identifier_dash = 1U << 5;
    constexpr unsigned flag_dollar_variable = 1U << 6;
    constexpr unsigned flag_case_insensitive = 1U << 7;
    constexpr unsigned flag_hash_needs_space = 1U << 8;
    constexpr unsigned flag_colon_property = 1U << 9;
    constexpr unsigned flag_markup = 1U << 10;
    constexpr unsigned flag_hash_color = 1U << 11;
    constexpr unsigned flag_preprocessor = 1U << 12;
    constexpr unsigned flag_lifetime = 1U << 13;
    constexpr unsigned flag_no_single_quote = 1U << 14;

    enum class lexer_kind : uint8_t {
        generic,
        markdown,
        diff
    };

    struct language_rule {
        const string_view* names = nullptr;
        const string_view* keywords = nullptr;
        const string_view* types = nullptr;
        const string_view* constants = nullptr;
        const string_view* line_comments = nullptr;
        string_view block_open;
        string_view block_close;
        unsigned flags = 0;
        lexer_kind kind = lexer_kind::generic;
    };

    // Every token table below is terminated by an empty view; no alias, keyword or constant is empty.
    constexpr string_view cpp_names[] = {"cpp", "c++", "cc", "cxx", "h", "hpp", "hxx", ""};
    constexpr string_view cpp_keywords[] = {"alignas",     "alignof",      "and",       "and_eq",
                                            "asm",         "auto",         "bitand",    "bitor",
                                            "break",       "case",         "catch",     "class",
                                            "compl",       "const",        "constexpr", "const_cast",
                                            "continue",    "decltype",     "default",   "delete",
                                            "do",          "dynamic_cast", "else",      "enum",
                                            "explicit",    "export",       "extern",    "final",
                                            "for",         "friend",       "goto",      "if",
                                            "inline",      "mutable",      "namespace", "new",
                                            "noexcept",    "not",          "not_eq",    "operator",
                                            "or",          "or_eq",        "override",  "private",
                                            "protected",   "public",       "register",  "reinterpret_cast",
                                            "return",      "sizeof",       "static",    "static_assert",
                                            "static_cast", "struct",       "switch",    "template",
                                            "this",        "thread_local", "throw",     "try",
                                            "typedef",     "typeid",       "typename",  "union",
                                            "using",       "virtual",      "volatile",  "while",
                                            "xor",         "xor_eq",       ""};
    constexpr string_view cpp_types[] = {"bool",     "char",     "char16_t", "char32_t", "double",  "float",
                                         "int",      "int8_t",   "int16_t",  "int32_t",  "int64_t", "long",
                                         "short",    "signed",   "size_t",   "ssize_t",  "uint8_t", "uint16_t",
                                         "uint32_t", "uint64_t", "unsigned", "void",     "wchar_t", ""};
    constexpr string_view cpp_constants[] = {"NULL", "false", "nullptr", "true", ""};
    constexpr string_view cpp_line_comments[] = {"//", ""};

    constexpr string_view c_names[] = {"c", ""};
    constexpr string_view c_keywords[] = {"_Alignas", "_Alignof",   "_Atomic",   "_Bool",          "_Complex",
                                          "_Generic", "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local",
                                          "auto",     "break",      "case",      "const",          "continue",
                                          "default",  "do",         "else",      "enum",           "extern",
                                          "for",      "goto",       "if",        "inline",         "register",
                                          "restrict", "return",     "sizeof",    "static",         "struct",
                                          "switch",   "typedef",    "union",     "volatile",       "while",
                                          ""};
    constexpr string_view c_types[] = {"char",   "double", "float",   "int",      "long", "short",
                                       "signed", "size_t", "ssize_t", "unsigned", "void", ""};
    constexpr string_view c_constants[] = {"NULL", "false", "true", ""};
    constexpr string_view c_line_comments[] = {"//", ""};

    constexpr string_view rust_names[] = {"rust", "rs", ""};
    constexpr string_view rust_keywords[] = {
            "as",     "async", "await",  "break", "const",  "continue", "crate", "dyn",    "else",   "enum",
            "extern", "fn",    "for",    "if",    "impl",   "in",       "let",   "loop",   "match",  "mod",
            "move",   "mut",   "pub",    "ref",   "return", "self",     "Self",  "static", "struct", "super",
            "trait",  "type",  "unsafe", "use",   "where",  "while",    ""};
    constexpr string_view rust_types[] = {"Box", "Option", "Result", "String", "Vec",  "bool",  "char",  "f32",
                                          "f64", "i8",     "i16",    "i32",    "i64",  "i128",  "isize", "str",
                                          "u8",  "u16",    "u32",    "u64",    "u128", "usize", ""};
    constexpr string_view rust_constants[] = {"Err", "None", "Ok", "Some", "false", "true", ""};
    constexpr string_view rust_line_comments[] = {"//", ""};

    constexpr string_view go_names[] = {"go", "golang", ""};
    constexpr string_view go_keywords[] = {"break",  "case",        "chan", "const",   "continue", "default", "defer",
                                           "else",   "fallthrough", "for",  "func",    "go",       "goto",    "if",
                                           "import", "interface",   "map",  "package", "range",    "return",  "select",
                                           "struct", "switch",      "type", "var",     ""};
    constexpr string_view go_types[] = {"bool", "byte",  "complex64", "complex128", "error",  "float32", "float64",
                                        "int",  "int8",  "int16",     "int32",      "int64",  "rune",    "string",
                                        "uint", "uint8", "uint16",    "uint32",     "uint64", "uintptr", ""};
    constexpr string_view go_constants[] = {"false", "iota", "nil", "true", ""};
    constexpr string_view go_line_comments[] = {"//", ""};

    constexpr string_view python_names[] = {"python", "py", ""};
    constexpr string_view python_keywords[] = {
            "and",      "as",     "assert",  "async", "await", "break",  "class", "continue", "def",  "del",   "elif",
            "else",     "except", "finally", "for",   "from",  "global", "if",    "import",   "in",   "is",    "lambda",
            "nonlocal", "not",    "or",      "pass",  "raise", "return", "try",   "while",    "with", "yield", ""};
    constexpr string_view python_types[] = {"bool",  "bytearray", "bytes", "complex", "dict",
                                            "float", "frozenset", "int",   "list",    "object",
                                            "set",   "str",       "tuple", "type",    ""};
    constexpr string_view python_constants[] = {"Ellipsis", "False", "None", "NotImplemented", "True", ""};
    constexpr string_view python_line_comments[] = {"#", ""};

    constexpr string_view js_names[] = {"javascript", "js", "jsx", "mjs", "cjs", ""};
    constexpr string_view js_keywords[] = {
            "async",    "await",    "break",  "case",   "catch",  "class",      "const",   "continue",
            "debugger", "default",  "delete", "do",     "else",   "export",     "extends", "finally",
            "for",      "function", "if",     "import", "in",     "instanceof", "let",     "new",
            "of",       "return",   "static", "super",  "switch", "this",       "throw",   "try",
            "typeof",   "var",      "void",   "while",  "with",   "yield",      ""};
    constexpr string_view js_types[] = {"Array", "BigInt", "Boolean", "Date",    "Error",   "JSON",
                                        "Map",   "Math",   "Number",  "Object",  "Promise", "RegExp",
                                        "Set",   "String", "Symbol",  "WeakMap", "WeakSet", ""};
    constexpr string_view js_constants[] = {"Infinity", "NaN", "false", "null", "true", "undefined", ""};
    constexpr string_view js_line_comments[] = {"//", ""};

    constexpr string_view ts_names[] = {"typescript", "ts", "tsx", ""};
    constexpr string_view ts_keywords[] = {
            "abstract", "as",         "asserts",   "async",   "await",    "break",  "case",       "catch",  "class",
            "const",    "continue",   "debugger",  "declare", "default",  "delete", "do",         "else",   "enum",
            "export",   "extends",    "finally",   "for",     "function", "if",     "implements", "import", "in",
            "infer",    "instanceof", "interface", "is",      "keyof",    "let",    "namespace",  "new",    "of",
            "override", "private",    "protected", "public",  "readonly", "return", "satisfies",  "static", "super",
            "switch",   "this",       "throw",     "try",     "type",     "typeof", "var",        "void",   "while",
            "with",     "yield",      ""};
    constexpr string_view ts_types[] = {"Array",  "BigInt",  "Boolean", "Date",    "Error",   "JSON",    "Map",
                                        "Math",   "Number",  "Object",  "Promise", "RegExp",  "Set",     "String",
                                        "Symbol", "WeakMap", "WeakSet", "any",     "bigint",  "boolean", "never",
                                        "number", "object",  "string",  "symbol",  "unknown", ""};
    constexpr string_view ts_constants[] = {"Infinity", "NaN", "false", "null", "true", "undefined", ""};
    constexpr string_view ts_line_comments[] = {"//", ""};

    constexpr string_view json_names[] = {"json", ""};
    constexpr string_view json_constants[] = {"false", "null", "true", ""};

    constexpr string_view yaml_names[] = {"yaml", "yml", ""};
    constexpr string_view yaml_constants[] = {"false", "no", "null", "off", "on", "true", "yes", ""};
    constexpr string_view yaml_line_comments[] = {"#", ""};

    constexpr string_view toml_names[] = {"toml", ""};
    constexpr string_view toml_constants[] = {"false", "true", ""};
    constexpr string_view toml_line_comments[] = {"#", ""};

    constexpr string_view bash_names[] = {"bash", "sh", "shell", "zsh", ""};
    constexpr string_view bash_keywords[] = {"case",   "coproc", "do",   "done",     "elif",  "else",
                                             "esac",   "fi",     "for",  "function", "if",    "in",
                                             "select", "then",   "time", "until",    "while", ""};
    constexpr string_view bash_constants[] = {"false", "true", ""};
    constexpr string_view bash_line_comments[] = {"#", ""};

    constexpr string_view sql_names[] = {"sql", ""};
    constexpr string_view sql_keywords[] = {
            "add",      "all",      "alter",   "and",     "as",         "begin",      "between", "by",
            "case",     "check",    "column",  "commit",  "constraint", "create",     "cross",   "cursor",
            "database", "declare",  "default", "delete",  "distinct",   "drop",       "else",    "end",
            "exists",   "fetch",    "foreign", "from",    "full",       "grant",      "group",   "having",
            "if",       "in",       "index",   "inner",   "insert",     "into",       "is",      "join",
            "key",      "left",     "like",    "limit",   "loop",       "not",        "offset",  "on",
            "or",       "order",    "outer",   "primary", "procedure",  "references", "return",  "revoke",
            "right",    "rollback", "schema",  "select",  "set",        "table",      "then",    "transaction",
            "trigger",  "union",    "unique",  "update",  "values",     "view",       "when",    "where",
            "while",    ""};
    constexpr string_view sql_types[] = {"bigint",    "blob",     "boolean", "char",    "clob",     "date", "datetime",
                                         "decimal",   "double",   "float",   "int",     "integer",  "json", "nchar",
                                         "numeric",   "nvarchar", "real",    "serial",  "smallint", "text", "time",
                                         "timestamp", "tinyint",  "uuid",    "varchar", ""};
    constexpr string_view sql_constants[] = {
            "current_date", "current_time", "current_timestamp", "false", "null", "true", ""};
    constexpr string_view sql_line_comments[] = {"--", ""};

    constexpr string_view html_names[] = {"html", "htm", "xml", "svg", ""};
    constexpr string_view css_names[] = {"css", ""};
    constexpr string_view css_keywords[] = {
            "absolute",  "auto",      "block",    "border-box", "charset", "content", "currentcolor", "fixed",
            "flex",      "font-face", "grid",     "important",  "import",  "inherit", "initial",      "inline",
            "keyframes", "layer",     "media",    "none",       "normal",  "page",    "relative",     "revert",
            "static",    "sticky",    "supports", "unset",      ""};
    constexpr string_view css_types[] = {"ch", "cm",  "deg", "em", "ex",   "fr", "grad", "in",   "ms", "pc", "pt",
                                         "px", "rad", "rem", "s",  "turn", "vh", "vmax", "vmin", "vw", ""};

    constexpr string_view markdown_names[] = {"markdown", "md", ""};
    constexpr string_view diff_names[] = {"diff", "patch", ""};

    constexpr string_view java_names[] = {"java", ""};
    constexpr string_view java_keywords[] = {
            "abstract", "assert",  "break",      "case",         "catch",      "class",     "const",   "continue",
            "default",  "do",      "else",       "enum",         "extends",    "final",     "finally", "for",
            "goto",     "if",      "implements", "import",       "instanceof", "interface", "native",  "new",
            "package",  "private", "protected",  "public",       "record",     "return",    "sealed",  "static",
            "strictfp", "super",   "switch",     "synchronized", "this",       "throw",     "throws",  "transient",
            "try",      "var",     "volatile",   "while",        "yield",      ""};
    constexpr string_view java_types[] = {"Boolean", "Byte",  "Character", "Double",  "Float", "Integer", "Long",
                                          "Object",  "Short", "String",    "boolean", "byte",  "char",    "double",
                                          "float",   "int",   "long",      "short",   "void",  ""};
    constexpr string_view java_constants[] = {"false", "null", "true", ""};
    constexpr string_view java_line_comments[] = {"//", ""};

    constexpr string_view kotlin_names[] = {"kotlin", "kt", ""};
    constexpr string_view kotlin_keywords[] = {
            "actual",    "abstract",  "annotation",  "as",        "break",       "by",      "catch",    "class",
            "companion", "const",     "constructor", "continue",  "crossinline", "data",    "delegate", "do",
            "dynamic",   "else",      "enum",        "expect",    "external",    "field",   "file",     "final",
            "finally",   "for",       "fun",         "get",       "if",          "import",  "in",       "infix",
            "init",      "inline",    "inner",       "interface", "internal",    "is",      "lateinit", "noinline",
            "object",    "open",      "operator",    "out",       "override",    "package", "param",    "private",
            "property",  "protected", "public",      "receiver",  "reified",     "return",  "sealed",   "set",
            "setparam",  "super",     "suspend",     "tailrec",   "this",        "throw",   "try",      "typealias",
            "typeof",    "val",       "var",         "vararg",    "when",        "where",   "while",    ""};
    constexpr string_view kotlin_types[] = {"Any",   "Array", "Boolean", "Byte", "Char", "Double",
                                            "Float", "Int",   "List",    "Long", "Map",  "Nothing",
                                            "Set",   "Short", "String",  "Unit", ""};
    constexpr string_view kotlin_constants[] = {"false", "null", "true", ""};
    constexpr string_view kotlin_line_comments[] = {"//", ""};

    constexpr string_view swift_names[] = {"swift", ""};
    constexpr string_view swift_keywords[] = {"associatedtype",
                                              "class",
                                              "deinit",
                                              "enum",
                                              "extension",
                                              "fileprivate",
                                              "func",
                                              "import",
                                              "init",
                                              "inout",
                                              "internal",
                                              "let",
                                              "open",
                                              "operator",
                                              "private",
                                              "protocol",
                                              "public",
                                              "rethrows",
                                              "static",
                                              "struct",
                                              "subscript",
                                              "typealias",
                                              "var",
                                              "break",
                                              "case",
                                              "continue",
                                              "default",
                                              "defer",
                                              "do",
                                              "else",
                                              "fallthrough",
                                              "for",
                                              "guard",
                                              "if",
                                              "in",
                                              "repeat",
                                              "return",
                                              "switch",
                                              "where",
                                              "while",
                                              "as",
                                              "catch",
                                              "is",
                                              "super",
                                              "self",
                                              "Self",
                                              "throw",
                                              "throws",
                                              "try",
                                              ""};
    constexpr string_view swift_types[] = {"Any",    "AnyObject", "Array",  "Bool",   "Character", "Dictionary",
                                           "Double", "Float",     "Int",    "Int8",   "Int16",     "Int32",
                                           "Int64",  "Optional",  "Set",    "String", "UInt",      "UInt8",
                                           "UInt16", "UInt32",    "UInt64", "Void",   ""};
    constexpr string_view swift_constants[] = {"false", "nil", "true", ""};
    constexpr string_view swift_line_comments[] = {"//", ""};

    constexpr string_view lua_names[] = {"lua", ""};
    constexpr string_view lua_keywords[] = {"and",      "break",  "do",   "else", "elseif", "end",   "false", "for",
                                            "function", "goto",   "if",   "in",   "local",  "nil",   "not",   "or",
                                            "repeat",   "return", "then", "true", "until",  "while", ""};
    constexpr string_view lua_constants[] = {"false", "nil", "true", ""};
    constexpr string_view lua_line_comments[] = {"--", ""};

    const language_rule* language_table() {
        static const language_rule rules[] = {
                {cpp_names, cpp_keywords, cpp_types, cpp_constants, cpp_line_comments, "/*", "*/", flag_preprocessor,
                 lexer_kind::generic},
                {c_names, c_keywords, c_types, c_constants, c_line_comments, "/*", "*/", flag_preprocessor,
                 lexer_kind::generic},
                {rust_names, rust_keywords, rust_types, rust_constants, rust_line_comments, "/*", "*/",
                 flag_lifetime | flag_colon_property, lexer_kind::generic},
                {go_names, go_keywords, go_types, go_constants, go_line_comments, "/*", "*/",
                 flag_backtick | flag_backtick_raw, lexer_kind::generic},
                {python_names, python_keywords, python_types, python_constants, python_line_comments, "", "",
                 flag_triple_single | flag_triple_double | flag_colon_property, lexer_kind::generic},
                {js_names, js_keywords, js_types, js_constants, js_line_comments, "/*", "*/", flag_backtick,
                 lexer_kind::generic},
                {ts_names, ts_keywords, ts_types, ts_constants, ts_line_comments, "/*", "*/", flag_backtick,
                 lexer_kind::generic},
                {json_names, nullptr, nullptr, json_constants, nullptr, "", "",
                 flag_colon_property | flag_no_single_quote, lexer_kind::generic},
                {yaml_names, nullptr, nullptr, yaml_constants, yaml_line_comments, "", "",
                 flag_colon_property | flag_identifier_dash | flag_hash_needs_space, lexer_kind::generic},
                {toml_names, nullptr, nullptr, toml_constants, toml_line_comments, "", "",
                 flag_triple_single | flag_triple_double | flag_identifier_dash, lexer_kind::generic},
                {bash_names, bash_keywords, nullptr, bash_constants, bash_line_comments, "", "",
                 flag_backtick | flag_dollar_variable | flag_hash_needs_space, lexer_kind::generic},
                {sql_names, sql_keywords, sql_types, sql_constants, sql_line_comments, "/*", "*/",
                 flag_case_insensitive | flag_doubled_quote, lexer_kind::generic},
                {html_names, nullptr, nullptr, nullptr, nullptr, "<!--", "-->", flag_markup | flag_identifier_dash,
                 lexer_kind::generic},
                {css_names, css_keywords, css_types, nullptr, nullptr, "/*", "*/",
                 flag_identifier_dash | flag_hash_color | flag_colon_property, lexer_kind::generic},
                {markdown_names, nullptr, nullptr, nullptr, nullptr, "", "", 0, lexer_kind::markdown},
                {diff_names, nullptr, nullptr, nullptr, nullptr, "", "", 0, lexer_kind::diff},
                {java_names, java_keywords, java_types, java_constants, java_line_comments, "/*", "*/",
                 flag_triple_double, lexer_kind::generic},
                {kotlin_names, kotlin_keywords, kotlin_types, kotlin_constants, kotlin_line_comments, "/*", "*/",
                 flag_triple_double | flag_colon_property, lexer_kind::generic},
                {swift_names, swift_keywords, swift_types, swift_constants, swift_line_comments, "/*", "*/",
                 flag_triple_double | flag_colon_property, lexer_kind::generic},
                {lua_names, lua_keywords, nullptr, lua_constants, lua_line_comments, "--[[", "]]", 0,
                 lexer_kind::generic},
        };
        static_assert(sizeof(rules) / sizeof(rules[0]) == 20, "language table must contain every family");
        return rules;
    }

    bool match_at(const string_view line, const size_t pos, const string_view token) noexcept {
        return !token.empty() && pos <= line.size() && token.size() <= line.size() - pos &&
               line.substr(pos, token.size()).equal_to(token);
    }

    void push_run(vector<syntax_run>& out, const size_t begin, const size_t end, const syntax_class cls) {
        if (begin < end) {
            out.push_back(syntax_run{begin, end, cls});
        }
    }

    void clear_state(highlight_state& state) noexcept {
        state.current = highlight_state::mode::normal;
        state.quote = 0;
        state.escaped = false;
    }

    bool in_list(const string_view* list, const string_view word, const bool ignore_case) noexcept {
        if (list == nullptr) {
            return false;
        }
        for (size_t i = 0; !list[i].empty(); ++i) {
            if (ignore_case ? word.compare_ignore_case(list[i]) == 0 : word.equal_to(list[i])) {
                return true;
            }
        }
        return false;
    }

    bool is_ident_start_char(const char c) noexcept { return is_alpha(c) || c == '_'; }

    bool is_ident_char(const char c) noexcept { return is_alpha_or_digit(c) || c == '_'; }

    bool is_ident_start(const language_rule& rule, const string_view line, const size_t pos) noexcept {
        const char c = line[pos];
        if (is_ident_start_char(c)) {
            return true;
        }
        if ((rule.flags & flag_identifier_dash) != 0 && c == '-' && pos + 1 < line.size()) {
            const char next = line[pos + 1];
            return is_alpha(next) || next == '_' || next == '-';
        }
        return false;
    }

    bool is_ident_continue(const language_rule& rule, const char c) noexcept {
        return is_ident_char(c) || ((rule.flags & flag_identifier_dash) != 0 && c == '-');
    }

    size_t skip_blanks(const string_view line, size_t pos) noexcept {
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
            ++pos;
        }
        return pos;
    }

    syntax_class identifier_class(const language_rule& rule, const string_view line, const size_t begin,
                                  const size_t end) noexcept {
        const string_view word = line.substr(begin, end - begin);
        const bool ignore_case = (rule.flags & flag_case_insensitive) != 0;
        if (in_list(rule.keywords, word, ignore_case)) {
            return syntax_class::keyword;
        }
        if (in_list(rule.types, word, ignore_case)) {
            return syntax_class::type;
        }
        if (in_list(rule.constants, word, ignore_case)) {
            return syntax_class::constant;
        }
        const size_t next = skip_blanks(line, end);
        if (next < line.size() && line[next] == '(') {
            return syntax_class::function;
        }
        if ((rule.flags & flag_colon_property) != 0 && next < line.size() && line[next] == ':' &&
            (next + 1 >= line.size() || line[next + 1] != ':')) {
            return syntax_class::variable;
        }
        if (next < line.size() && line[next] == '=') {
            const char after = next + 1 < line.size() ? line[next + 1] : '\0';
            if (after != '=' && after != '>' && after != '~') {
                return syntax_class::variable;
            }
        }
        return syntax_class::plain;
    }

    bool is_operator_char(const char c) noexcept {
        switch (c) {
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case '=':
            case '<':
            case '>':
            case '!':
            case '&':
            case '|':
            case '^':
            case '~':
            case '?':
            case '$':
                return true;
            default:
                return false;
        }
    }

    bool is_punctuation_char(const char c) noexcept {
        switch (c) {
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case ',':
            case ';':
            case ':':
            case '.':
            case '@':
            case '#':
            case '\\':
                return true;
            default:
                return false;
        }
    }

    bool is_number_suffix(const char c) noexcept {
        switch (c) {
            case 'u':
            case 'U':
            case 'l':
            case 'L':
            case 'f':
            case 'F':
            case 'd':
            case 'D':
            case 'j':
            case 'J':
                return true;
            default:
                return false;
        }
    }

    size_t scan_radix_number(const string_view line, const size_t start, const unsigned radix) noexcept {
        const size_t length = line.size();
        size_t pos = start + 2;
        bool has_digit = false;
        while (pos < length) {
            const char c = line[pos];
            bool valid = false;
            if (radix == 16) {
                valid = static_cast<int>(is_xdigit(c)) != 0;
            } else if (radix == 8) {
                valid = c >= '0' && c <= '7';
            } else {
                valid = c == '0' || c == '1';
            }
            if (!valid && c != '_') {
                break;
            }
            has_digit = has_digit || valid;
            ++pos;
        }
        if (!has_digit) {
            return start + 1;
        }
        while (pos < length && is_number_suffix(line[pos])) {
            ++pos;
        }
        return pos;
    }

    size_t scan_number(const string_view line, const size_t start) noexcept {
        const size_t length = line.size();
        if (line[start] == '0' && start + 1 < length) {
            const char second = line[start + 1];
            if (second == 'x' || second == 'X') {
                return scan_radix_number(line, start, 16);
            }
            if (second == 'b' || second == 'B') {
                return scan_radix_number(line, start, 2);
            }
            if (second == 'o' || second == 'O') {
                return scan_radix_number(line, start, 8);
            }
        }
        size_t pos = start;
        while (pos < length && (is_digit(line[pos]) || line[pos] == '_')) {
            ++pos;
        }
        if (pos + 1 < length && line[pos] == '.' && is_digit(line[pos + 1])) {
            ++pos;
            while (pos < length && (is_digit(line[pos]) || line[pos] == '_')) {
                ++pos;
            }
        }
        if (pos < length && (line[pos] == 'e' || line[pos] == 'E')) {
            size_t exponent = pos + 1;
            if (exponent < length && (line[exponent] == '+' || line[exponent] == '-')) {
                ++exponent;
            }
            if (exponent < length && is_digit(line[exponent])) {
                pos = exponent + 1;
                while (pos < length && (is_digit(line[pos]) || line[pos] == '_')) {
                    ++pos;
                }
            }
        }
        while (pos < length && is_number_suffix(line[pos])) {
            ++pos;
        }
        return pos;
    }

    struct scan_result {
        size_t end = 0;
        bool closed = false;
        bool escaped = false;
    };

    // TODO: support C++/Rust raw string literals and nested block comments for Rust, Kotlin and Swift.
    scan_result scan_quoted_body(const string_view line, size_t pos, const char quote, const size_t close_len,
                                 const bool allow_escape, const bool doubled_quote) noexcept {
        const size_t length = line.size();
        while (pos < length) {
            const char c = line[pos];
            if (allow_escape && c == '\\') {
                if (pos + 1 >= length) {
                    return scan_result{length, false, true};
                }
                pos += 2;
                continue;
            }
            if (c == quote) {
                if (close_len == 3) {
                    if (length - pos >= 3 && line[pos + 1] == quote && line[pos + 2] == quote) {
                        return scan_result{pos + 3, true, false};
                    }
                    ++pos;
                    continue;
                }
                if (doubled_quote && length - pos >= 2 && line[pos + 1] == quote) {
                    pos += 2;
                    continue;
                }
                return scan_result{pos + 1, true, false};
            }
            ++pos;
        }
        return scan_result{length, false, false};
    }

    bool supports_quote(const language_rule& rule, const char quote) noexcept {
        if (quote == '"') {
            return true;
        }
        if (quote == '\'') {
            return (rule.flags & flag_no_single_quote) == 0;
        }
        if (quote == '`') {
            return (rule.flags & flag_backtick) != 0;
        }
        return false;
    }

    void lex_string(const string_view line, size_t& pos, const char quote, const language_rule& rule,
                    highlight_state& state, vector<syntax_run>& out) {
        const size_t begin = pos;
        const size_t length = line.size();
        const bool triple = (quote == '"' && (rule.flags & flag_triple_double) != 0 && length - begin >= 3 &&
                             line[begin + 1] == quote && line[begin + 2] == quote) ||
                            (quote == '\'' && (rule.flags & flag_triple_single) != 0 && length - begin >= 3 &&
                             line[begin + 1] == quote && line[begin + 2] == quote);
        const size_t open_len = triple ? 3 : 1;
        const size_t close_len = triple ? 3 : 1;
        const bool allow_escape = quote != '`' || (rule.flags & flag_backtick_raw) == 0;
        const bool doubled_quote = (rule.flags & flag_doubled_quote) != 0;
        const scan_result result =
                scan_quoted_body(line, begin + open_len, quote, close_len, allow_escape, doubled_quote);
        auto cls = syntax_class::string;
        if ((rule.flags & flag_colon_property) != 0 && result.closed) {
            const size_t next = skip_blanks(line, result.end);
            if (next < length && line[next] == ':' && (next + 1 >= length || line[next + 1] != ':')) {
                cls = syntax_class::variable;
            }
        }
        push_run(out, begin, result.end, cls);
        pos = result.end;
        if (!result.closed && (triple || quote == '`')) {
            state.current = highlight_state::mode::string_literal;
            state.quote = quote;
            state.escaped = result.escaped;
        } else {
            clear_state(state);
        }
    }

    bool line_comment_at(const language_rule& rule, const string_view line, const size_t pos) noexcept {
        if (rule.line_comments == nullptr) {
            return false;
        }
        for (size_t i = 0; !rule.line_comments[i].empty(); ++i) {
            const string_view prefix = rule.line_comments[i];
            if (!match_at(line, pos, prefix)) {
                continue;
            }
            if ((rule.flags & flag_hash_needs_space) != 0 && prefix.size() == 1 && prefix[0] == '#' && pos > 0) {
                const char prev = line[pos - 1];
                if (prev != ' ' && prev != '\t' && prev != ';' && prev != '(' && prev != '{' && prev != '&' &&
                    prev != '|') {
                    continue;
                }
            }
            return true;
        }
        return false;
    }

    bool only_blanks_before(const string_view line, const size_t pos) noexcept {
        for (size_t i = 0; i < pos; ++i) {
            if (line[i] != ' ' && line[i] != '\t') {
                return false;
            }
        }
        return true;
    }

    bool is_special_shell_char(const char c) noexcept {
        return c == '?' || c == '#' || c == '@' || c == '*' || c == '$' || c == '!' || c == '-';
    }

    void highlight_generic(const string_view line, const language_rule& rule, highlight_state& state,
                           vector<syntax_run>& out) {
        out.clear();
        const size_t length = line.size();
        size_t pos = 0;

        if (state.current == highlight_state::mode::normal) {
            state.quote = 0;
            state.escaped = false;
        }

        if (state.current == highlight_state::mode::block_comment) {
            if (rule.block_open.empty() || rule.block_close.empty()) {
                clear_state(state);
            } else {
                const size_t close = line.find(rule.block_close);
                if (close == string_view::npos) {
                    push_run(out, 0, length, syntax_class::comment);
                    return;
                }
                push_run(out, 0, close + rule.block_close.size(), syntax_class::comment);
                pos = close + rule.block_close.size();
                clear_state(state);
            }
        } else if (state.current == highlight_state::mode::string_literal) {
            if (!supports_quote(rule, state.quote)) {
                clear_state(state);
            } else {
                const bool triple = (state.quote == '"' && (rule.flags & flag_triple_double) != 0) ||
                                    (state.quote == '\'' && (rule.flags & flag_triple_single) != 0);
                const bool allow_escape = state.quote != '`' || (rule.flags & flag_backtick_raw) == 0;
                size_t body = 0;
                if (state.escaped && body < length) {
                    body = 1;
                }
                const scan_result result = scan_quoted_body(line, body, state.quote, triple ? 3 : 1, allow_escape,
                                                            (rule.flags & flag_doubled_quote) != 0);
                if (!result.closed) {
                    push_run(out, 0, length, syntax_class::string);
                    state.escaped = result.escaped;
                    return;
                }
                push_run(out, 0, result.end, syntax_class::string);
                pos = result.end;
                clear_state(state);
            }
        }

        bool in_tag = false;
        bool tag_name = false;

        while (pos < length) {
            const char c = line[pos];

            if (!rule.block_open.empty() && match_at(line, pos, rule.block_open)) {
                const size_t close = line.find(rule.block_close, pos + rule.block_open.size());
                if (close == string_view::npos) {
                    push_run(out, pos, length, syntax_class::comment);
                    state.current = highlight_state::mode::block_comment;
                    state.quote = 0;
                    state.escaped = false;
                    return;
                }
                push_run(out, pos, close + rule.block_close.size(), syntax_class::comment);
                pos = close + rule.block_close.size();
                continue;
            }

            if (line_comment_at(rule, line, pos)) {
                push_run(out, pos, length, syntax_class::comment);
                return;
            }

            if ((rule.flags & flag_lifetime) != 0 && c == '\'' && pos + 1 < length &&
                is_ident_start_char(line[pos + 1]) && (pos + 2 >= length || line[pos + 2] != '\'')) {
                size_t end = pos + 1;
                while (end < length && is_ident_char(line[end])) {
                    ++end;
                }
                push_run(out, pos, end, syntax_class::variable);
                pos = end;
                continue;
            }

            if (c == '"') {
                lex_string(line, pos, c, rule, state, out);
                continue;
            }
            if (c == '\'' && (rule.flags & flag_no_single_quote) == 0) {
                lex_string(line, pos, c, rule, state, out);
                continue;
            }
            if (c == '`' && (rule.flags & flag_backtick) != 0) {
                lex_string(line, pos, c, rule, state, out);
                continue;
            }

            if (c == '$' && (rule.flags & flag_dollar_variable) != 0) {
                size_t end = pos + 1;
                if (end < length && line[end] == '{') {
                    const size_t close = line.find('}', end + 1);
                    end = close == string_view::npos ? length : close + 1;
                } else if (end < length && is_ident_start_char(line[end])) {
                    ++end;
                    while (end < length && is_ident_char(line[end])) {
                        ++end;
                    }
                } else if (end < length && (is_digit(line[end]) || is_special_shell_char(line[end]))) {
                    ++end;
                }
                if (end > pos + 1) {
                    push_run(out, pos, end, syntax_class::variable);
                    pos = end;
                    continue;
                }
            }

            if (c == '#' && (rule.flags & flag_preprocessor) != 0 && only_blanks_before(line, pos)) {
                size_t end = pos + 1;
                if (end < length && is_ident_start_char(line[end])) {
                    ++end;
                    while (end < length && is_ident_char(line[end])) {
                        ++end;
                    }
                }
                push_run(out, pos, end, syntax_class::keyword);
                pos = end;
                continue;
            }

            if ((rule.flags & flag_markup) != 0) {
                if (c == '<') {
                    push_run(out, pos, pos + 1, syntax_class::punctuation);
                    ++pos;
                    in_tag = true;
                    tag_name = true;
                    if (pos < length && (line[pos] == '/' || line[pos] == '!' || line[pos] == '?')) {
                        push_run(out, pos, pos + 1, syntax_class::punctuation);
                        ++pos;
                    }
                    continue;
                }
                if (in_tag && c == '>') {
                    push_run(out, pos, pos + 1, syntax_class::punctuation);
                    ++pos;
                    in_tag = false;
                    tag_name = false;
                    continue;
                }
            }

            if (c == '#' && (rule.flags & flag_hash_color) != 0 && pos + 1 < length && is_xdigit(line[pos + 1])) {
                size_t end = pos + 1;
                while (end < length && is_xdigit(line[end])) {
                    ++end;
                }
                push_run(out, pos, end, syntax_class::number);
                pos = end;
                continue;
            }

            if (is_digit(c)) {
                const size_t end = scan_number(line, pos);
                push_run(out, pos, end, syntax_class::number);
                pos = end;
                continue;
            }

            if (is_ident_start(rule, line, pos)) {
                size_t end = pos;
                while (end < length && is_ident_continue(rule, line[end])) {
                    ++end;
                }
                auto cls = syntax_class::plain;
                if (in_tag) {
                    cls = tag_name ? syntax_class::keyword : syntax_class::variable;
                    tag_name = false;
                } else {
                    cls = identifier_class(rule, line, pos, end);
                }
                if (cls != syntax_class::plain) {
                    push_run(out, pos, end, cls);
                }
                pos = end;
                continue;
            }

            if (is_operator_char(c)) {
                push_run(out, pos, pos + 1, syntax_class::op);
                ++pos;
                continue;
            }
            if (is_punctuation_char(c)) {
                push_run(out, pos, pos + 1, syntax_class::punctuation);
                ++pos;
                continue;
            }
            ++pos;
        }
    }

    void highlight_markdown(const string_view line, highlight_state& state, vector<syntax_run>& out) {
        out.clear();
        clear_state(state);
        const size_t length = line.size();
        size_t pos = 0;
        while (pos < length && (line[pos] == ' ' || line[pos] == '\t')) {
            ++pos;
        }

        if (pos < length && line[pos] == '#') {
            size_t hashes = pos;
            while (hashes < length && line[hashes] == '#') {
                ++hashes;
            }
            if (hashes - pos <= 6 && (hashes == length || line[hashes] == ' ' || line[hashes] == '\t')) {
                push_run(out, pos, hashes, syntax_class::keyword);
                size_t text = hashes;
                while (text < length && (line[text] == ' ' || line[text] == '\t')) {
                    ++text;
                }
                push_run(out, text, length, syntax_class::type);
                return;
            }
        }

        // TODO: track fenced code blocks across lines so their content stays plain.
        if (pos + 2 < length && (line[pos] == '`' || line[pos] == '~')) {
            const char fence = line[pos];
            size_t end = pos;
            while (end < length && line[end] == fence) {
                ++end;
            }
            if (end - pos >= 3) {
                push_run(out, pos, length, syntax_class::keyword);
                return;
            }
        }

        if (pos < length) {
            const char c = line[pos];
            if ((c == '-' || c == '*' || c == '+') && pos + 1 < length &&
                (line[pos + 1] == ' ' || line[pos + 1] == '\t')) {
                push_run(out, pos, pos + 1, syntax_class::keyword);
                ++pos;
            } else if (is_digit(c)) {
                size_t end = pos;
                while (end < length && is_digit(line[end])) {
                    ++end;
                }
                if (end < length && (line[end] == '.' || line[end] == ')') && end + 1 < length &&
                    (line[end + 1] == ' ' || line[end + 1] == '\t')) {
                    push_run(out, pos, end + 1, syntax_class::keyword);
                    pos = end + 1;
                }
            } else if (c == '>') {
                push_run(out, pos, pos + 1, syntax_class::punctuation);
                ++pos;
            }
        }

        while (pos < length) {
            if (line[pos] == '`') {
                const size_t close = line.find('`', pos + 1);
                const size_t end = close == string_view::npos ? length : close + 1;
                push_run(out, pos, end, syntax_class::string);
                pos = end;
                continue;
            }
            if (pos + 1 < length &&
                ((line[pos] == '*' && line[pos + 1] == '*') || (line[pos] == '_' && line[pos + 1] == '_') ||
                 (line[pos] == '~' && line[pos + 1] == '~'))) {
                push_run(out, pos, pos + 2, syntax_class::op);
                pos += 2;
                continue;
            }
            if (line[pos] == '*' || line[pos] == '_' || line[pos] == '~') {
                push_run(out, pos, pos + 1, syntax_class::op);
                ++pos;
                continue;
            }
            ++pos;
        }
    }

    void highlight_diff(const string_view line, highlight_state& state, vector<syntax_run>& out) {
        out.clear();
        clear_state(state);
        if (line.empty()) {
            return;
        }
        if (line.size() >= 2 && line[0] == '@' && line[1] == '@') {
            push_run(out, 0, line.size(), syntax_class::keyword);
            return;
        }
        if (line[0] == '+') {
            push_run(out, 0, line.size(), syntax_class::string);
            return;
        }
        if (line[0] == '-') {
            push_run(out, 0, line.size(), syntax_class::comment);
        }
    }

    class table_highlighter final : public highlighter {
    public:
        explicit table_highlighter(const language_rule* rule) noexcept :
        rule_(rule) {}

        void highlight_line(string_view line, highlight_state& state, vector<syntax_run>& out) const override;

        NEFORCE_NODISCARD const language_rule& rule() const noexcept { return *rule_; }

    private:
        const language_rule* rule_;
    };

    void table_highlighter::highlight_line(const string_view line, highlight_state& state,
                                           vector<syntax_run>& out) const {
        switch (rule_->kind) {
            case lexer_kind::markdown:
                highlight_markdown(line, state, out);
                break;
            case lexer_kind::diff:
                highlight_diff(line, state, out);
                break;
            default:
                highlight_generic(line, *rule_, state, out);
                break;
        }
    }

    const table_highlighter* highlighter_table(size_t& count) {
        const language_rule* rules = language_table();
        static const table_highlighter items[] = {
                table_highlighter(&rules[0]),  table_highlighter(&rules[1]),  table_highlighter(&rules[2]),
                table_highlighter(&rules[3]),  table_highlighter(&rules[4]),  table_highlighter(&rules[5]),
                table_highlighter(&rules[6]),  table_highlighter(&rules[7]),  table_highlighter(&rules[8]),
                table_highlighter(&rules[9]),  table_highlighter(&rules[10]), table_highlighter(&rules[11]),
                table_highlighter(&rules[12]), table_highlighter(&rules[13]), table_highlighter(&rules[14]),
                table_highlighter(&rules[15]), table_highlighter(&rules[16]), table_highlighter(&rules[17]),
                table_highlighter(&rules[18]), table_highlighter(&rules[19]),
        };
        static_assert(sizeof(items) / sizeof(items[0]) == 20, "registry must mirror the language table");
        count = sizeof(items) / sizeof(items[0]);
        return items;
    }
} // namespace


highlighter::~highlighter() = default;

const highlighter* find_highlighter(const string_view language) {
    string_view name = language;
    size_t begin = 0;
    size_t end = name.size();
    while (begin < end && (name[begin] == ' ' || name[begin] == '\t' || name[begin] == '\r' || name[begin] == '\n')) {
        ++begin;
    }
    while (end > begin &&
           (name[end - 1] == ' ' || name[end - 1] == '\t' || name[end - 1] == '\r' || name[end - 1] == '\n')) {
        --end;
    }
    name = name.substr(begin, end - begin);
    if (!name.empty() && name[0] == '.') {
        name = name.substr(1);
    }
    if (name.empty()) {
        return nullptr;
    }

    size_t count = 0;
    const table_highlighter* items = highlighter_table(count);
    for (size_t i = 0; i < count; ++i) {
        const string_view* names = items[i].rule().names;
        for (size_t k = 0; !names[k].empty(); ++k) {
            if (name.compare_ignore_case(names[k]) == 0) {
                return &items[i];
            }
        }
    }
    return nullptr;
}

size_t highlighter_count() noexcept {
    size_t count = 0;
    highlighter_table(count);
    return count;
}

string_view highlighter_language(const size_t index) {
    size_t count = 0;
    const table_highlighter* items = highlighter_table(count);
    if (index >= count) {
        return string_view{};
    }
    return items[index].rule().names[0];
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
