#include <NeForce/core/file/md/markdown_value.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    char lower_of(const char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

    string normalise_label(const string_view label) {
        string result;
        bool pending_space = false;
        for (size_t i = 0; i < label.size(); ++i) {
            const char c = label[i];
            if (is_space(c)) {
                if (!result.empty()) {
                    pending_space = true;
                }
                continue;
            }
            if (pending_space) {
                result += ' ';
                pending_space = false;
            }
            result += lower_of(c);
        }
        return result;
    }

    void append_plain(const md_inlines& inlines, string& out) {
        for (const auto& node: inlines) {
            switch (node.kind) {
                case md_inline_kind::text:
                case md_inline_kind::code:
                case md_inline_kind::image:
                case md_inline_kind::autolink:
                case md_inline_kind::html: {
                    out += node.text;
                    break;
                }
                case md_inline_kind::line_break: {
                    out += '\n';
                    break;
                }
                default: {
                    append_plain(node.children, out);
                    break;
                }
            }
        }
    }

    void append_cells(const md_list<md_inlines>& cells, string& out) {
        for (size_t i = 0; i < cells.size(); ++i) {
            if (i > 0) {
                out += '\t';
            }
            append_plain(cells[i], out);
        }
        out += '\n';
    }

    void append_block_plain(const md_block& block, string& out) {
        switch (block.kind) {
            case md_block_kind::paragraph:
            case md_block_kind::heading: {
                append_plain(block.inlines, out);
                out += '\n';
                break;
            }
            case md_block_kind::code_block:
            case md_block_kind::html_block: {
                out += block.literal;
                if (!block.literal.empty() && block.literal.back() != '\n') {
                    out += '\n';
                }
                break;
            }
            case md_block_kind::thematic_break: {
                break;
            }
            case md_block_kind::block_quote:
            case md_block_kind::list:
            case md_block_kind::list_item: {
                for (const auto& child: block.children) {
                    append_block_plain(child, out);
                }
                break;
            }
            case md_block_kind::table: {
                append_cells(block.header, out);
                for (const auto& row: block.rows) {
                    append_cells(row, out);
                }
                break;
            }
        }
    }

    string_view copy_string(bump_arena& arena, const string_view text) {
        if (text.empty()) {
            return string_view();
        }
        auto* data = static_cast<char*>(arena.allocate(text.size(), 1, 1));
        memory_copy(data, text.data(), text.size());
        return string_view(data, text.size());
    }
} // namespace


NEFORCE_BEGIN_INNER__

bump_arena*& md_arena_slot() noexcept {
    thread_local bump_arena* slot = nullptr;
    return slot;
}

bump_arena& md_fallback_arena() noexcept {
    thread_local bump_arena arena;
    return arena;
}

bump_arena& md_inline_arena() noexcept {
    thread_local bump_arena arena;
    return arena;
}

NEFORCE_END_INNER__


void markdown_document::set_source(const string_view text) { source_ = copy_string(arena_, text); }

md_inline markdown_document::clone_inline(const md_inline& inline_node) {
    md_inline result = inline_node;
    result.text = copy_string(arena_, inline_node.text);
    result.destination = copy_string(arena_, inline_node.destination);
    result.title = copy_string(arena_, inline_node.title);
    result.reference = copy_string(arena_, inline_node.reference);
    result.children = clone_inlines(inline_node.children);
    return result;
}

md_inlines markdown_document::clone_inlines(const md_inlines& inlines) {
    inner::md_arena_scope scope(&arena_);
    md_inlines result;
    result.reserve(inlines.size());
    for (const auto& node: inlines) {
        result.push_back(clone_inline(node));
    }
    return result;
}

md_block markdown_document::clone_block(const md_block& block) {
    inner::md_arena_scope scope(&arena_);
    md_block result = block;
    result.info = copy_string(arena_, block.info);
    result.language = copy_string(arena_, block.language);
    result.literal = copy_string(arena_, block.literal);

    result.alignments.clear();
    result.alignments.reserve(block.alignments.size());
    for (const auto alignment: block.alignments) {
        result.alignments.push_back(alignment);
    }

    result.header.clear();
    result.header.reserve(block.header.size());
    for (const auto& cell: block.header) {
        result.header.push_back(clone_inlines(cell));
    }

    result.rows.clear();
    result.rows.reserve(block.rows.size());
    for (const auto& row: block.rows) {
        md_list<md_inlines> cloned_row;
        cloned_row.reserve(row.size());
        for (const auto& cell: row) {
            cloned_row.push_back(clone_inlines(cell));
        }
        result.rows.push_back(cloned_row);
    }

    result.inlines = clone_inlines(block.inlines);
    result.children.clear();
    result.children.reserve(block.children.size());
    for (const auto& child: block.children) {
        result.children.push_back(clone_block(child));
    }
    return result;
}

string md_plain_text(const md_inlines& inlines) {
    string result;
    append_plain(inlines, result);
    return result;
}

string md_plain_text(const md_block& block) {
    string result;
    append_block_plain(block, result);
    return result;
}

bool md_empty(const markdown_document& document) noexcept {
    return document.blocks.empty() && document.references.empty();
}

const md_reference* md_find_reference(const markdown_document& document, const string_view label) {
    const string normalised = normalise_label(label);
    for (const auto& reference: document.references) {
        if (normalise_label(reference.label) == normalised.view()) {
            return &reference;
        }
    }
    return nullptr;
}

NEFORCE_END_NAMESPACE__
