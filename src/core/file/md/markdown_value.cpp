#include <NeForce/core/file/md/markdown_value.hpp>
#include <NeForce/core/hash/fnv.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    char lower_of(const char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

    uint64_t hash_label(const string_view label) noexcept { return FNV_hash_string64(label.data(), label.size()); }

    uint64_t normalised_hash(const string_view label) noexcept {
        uint64_t hash = 1469598103934665603ULL;
        bool pending_space = false;
        bool any = false;
        for (const char c: label) {
            if (is_space(c)) {
                if (any) {
                    pending_space = true;
                }
                continue;
            }
            if (pending_space) {
                hash ^= static_cast<uint8_t>(' ');
                hash *= 1099511628211ULL;
                pending_space = false;
            }
            hash ^= static_cast<uint8_t>(lower_of(c));
            hash *= 1099511628211ULL;
            any = true;
        }
        return hash;
    }

    bool normalised_equal(const string_view label, const string_view normalized) noexcept {
        size_t pos = 0;
        bool pending_space = false;
        bool any = false;
        for (const char c: label) {
            if (is_space(c)) {
                if (any) {
                    pending_space = true;
                }
                continue;
            }
            if (pending_space) {
                if (pos >= normalized.size() || normalized[pos] != ' ') {
                    return false;
                }
                ++pos;
                pending_space = false;
            }
            if (pos >= normalized.size() || normalized[pos] != lower_of(c)) {
                return false;
            }
            ++pos;
            any = true;
        }
        return pos == normalized.size();
    }

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
                case md_inline_kind::image: {
                    out += node.alt();
                    break;
                }
                case md_inline_kind::autolink:
                case md_inline_kind::html: {
                    out += node.text;
                    break;
                }
                case md_inline_kind::link: {
                    append_plain(node.link_children(), out);
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
                const string_view literal = block.literal();
                out += literal;
                if (!literal.empty() && literal.back() != '\n') {
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
                append_cells(block.header(), out);
                for (const auto& row: block.rows()) {
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


void markdown_document::set_source(const string_view text) {
    source_owned_.clear();
    source_ = copy_string(arena_, text);
}

void markdown_document::set_source(string&& text) {
    source_owned_ = move(text);
    if (source_owned_.capacity() < 16) {
        source_owned_.reserve(16);
    }
    source_ = source_owned_.view();
}

markdown_document::markdown_document(markdown_document&& other) noexcept :
arena_(move(other.arena_)),
source_(other.source_),
source_owned_(move(other.source_owned_)),
blocks(other.blocks),
references(other.references) {}

markdown_document& markdown_document::operator=(markdown_document&& other) noexcept {
    if (this != &other) {
        arena_ = move(other.arena_);
        source_ = other.source_;
        source_owned_ = move(other.source_owned_);
        blocks = other.blocks;
        references = other.references;
    }
    return *this;
}

md_inline markdown_document::clone_inline(const md_inline& inline_node) {
    md_inline result;
    result.kind = inline_node.kind;
    result.flags = inline_node.flags;
    switch (inline_node.kind) {
        case md_inline_kind::strong:
        case md_inline_kind::emphasis:
        case md_inline_kind::strikethrough: {
            result.children = clone_inlines(inline_node.children);
            break;
        }
        case md_inline_kind::link:
        case md_inline_kind::image: {
            auto* link = static_cast<md_link_data*>(arena_.allocate(1, sizeof(md_link_data), alignof(md_link_data)));
            link->destination = copy_string(arena_, inline_node.destination());
            link->title = copy_string(arena_, inline_node.title());
            link->reference = copy_string(arena_, inline_node.reference());
            if (inline_node.kind == md_inline_kind::image) {
                link->alt = copy_string(arena_, inline_node.alt());
            } else {
                link->children = clone_inlines(inline_node.link_children());
            }
            result.link = link;
            break;
        }
        default: {
            result.text = copy_string(arena_, inline_node.text);
            break;
        }
    }
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
    md_block result;
    result.kind = block.kind;
    result.level = block.level;
    result.setext = block.setext;
    result.ordered = block.ordered;
    result.start = block.start;
    result.delimiter = block.delimiter;
    result.bullet = block.bullet;
    result.tight = block.tight;
    result.number = block.number;
    result.task = block.task;
    result.checked = block.checked;

    switch (block.kind) {
        case md_block_kind::paragraph:
        case md_block_kind::heading: {
            result.inlines = clone_inlines(block.inlines);
            break;
        }
        case md_block_kind::block_quote:
        case md_block_kind::list:
        case md_block_kind::list_item: {
            result.children.reserve(block.children.size());
            for (const auto& child: block.children) {
                result.children.push_back(clone_block(child));
            }
            break;
        }
        case md_block_kind::code_block:
        case md_block_kind::html_block: {
            auto* code = static_cast<md_code_data*>(arena_.allocate(1, sizeof(md_code_data), alignof(md_code_data)));
            code->literal = copy_string(arena_, block.literal());
            code->info = copy_string(arena_, block.info());
            code->language = copy_string(arena_, block.language());
            code->fence_char = block.fence_char();
            code->fence_length = block.fence_length();
            code->html_type = block.html_type();
            code->fenced = block.fenced();
            result.code = code;
            break;
        }
        case md_block_kind::table: {
            auto* table =
                    static_cast<md_table_data*>(arena_.allocate(1, sizeof(md_table_data), alignof(md_table_data)));
            table->alignments.reserve(block.alignments().size());
            for (const auto alignment: block.alignments()) {
                table->alignments.push_back(alignment);
            }
            table->header.reserve(block.header().size());
            for (const auto& cell: block.header()) {
                table->header.push_back(clone_inlines(cell));
            }
            table->rows.reserve(block.rows().size());
            for (const auto& row: block.rows()) {
                md_list<md_inlines> cloned_row;
                cloned_row.reserve(row.size());
                for (const auto& cell: row) {
                    cloned_row.push_back(clone_inlines(cell));
                }
                table->rows.push_back(cloned_row);
            }
            result.table = table;
            break;
        }
        case md_block_kind::thematic_break: {
            break;
        }
    }
    return result;
}

const md_reference* markdown_document::find_reference(const string_view label) const {
    if (references.size() <= 8) {
        for (const auto& reference: references) {
            if (!reference.normalized.empty() ? normalised_equal(label, reference.normalized)
                                              : normalise_label(reference.label) == normalise_label(label).view()) {
                return &reference;
            }
        }
        return nullptr;
    }
    if (reference_slots_ == nullptr) {
        for (const auto& reference: references) {
            if (reference.normalized.empty()) {
                return nullptr;
            }
        }
        size_t capacity = 16;
        while (capacity < references.size() * 2) {
            capacity *= 2;
        }
        auto* slots = static_cast<uint32_t*>(
                const_cast<bump_arena&>(arena_).allocate(capacity, sizeof(uint32_t), alignof(uint32_t)));
        memory_zero(slots, capacity * sizeof(uint32_t));
        for (size_t i = 0; i < references.size(); ++i) {
            size_t slot = static_cast<size_t>(hash_label(references[i].normalized)) & (capacity - 1);
            while (slots[slot] != 0) {
                slot = (slot + 1) & (capacity - 1);
            }
            slots[slot] = static_cast<uint32_t>(i + 1);
        }
        reference_slots_ = slots;
        reference_capacity_ = static_cast<uint32_t>(capacity);
    }
    if (reference_capacity_ == 0) {
        return nullptr;
    }
    size_t slot = static_cast<size_t>(normalised_hash(label)) & (reference_capacity_ - 1);
    while (reference_slots_[slot] != 0) {
        const auto& reference = references[reference_slots_[slot] - 1];
        if (normalised_equal(label, reference.normalized)) {
            return &reference;
        }
        slot = (slot + 1) & (reference_capacity_ - 1);
    }
    return nullptr;
}

string md_normalise_label(const string_view label) { return normalise_label(label); }

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
    return document.find_reference(label);
}

NEFORCE_END_NAMESPACE__
