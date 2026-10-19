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

    void append_cells(const vector<md_inlines>& cells, string& out) {
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
} // namespace


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
        if (normalise_label(reference.label.view()) == normalised.view()) {
            return &reference;
        }
    }
    return nullptr;
}

NEFORCE_END_NAMESPACE__
