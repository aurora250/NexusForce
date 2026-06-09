#include <NeForce/network/http/multipart_parser.hpp>
#include <NeForce/core/string/utf.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    bool parse_part_headers(string_view part, size_t& offset, multipart_field& field) {
        while (offset < part.size()) {
            auto eol = part.find("\r\n", offset);
            if (eol == string_view::npos || eol == offset) {
                offset = (eol == offset) ? eol + 2 : offset;
                break;
            }

            string_view line = part.substr(offset, eol - offset);
            offset = eol + 2;

            auto colon = line.find(':');
            if (colon == string_view::npos) {
                continue;
            }

            const string key = line.substr(0, colon);
            string_view value = line.substr(colon + 1);

            while (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }

            if (key.lowercase() == "content-disposition") {
                auto name_pos = value.find("name=\"");
                if (name_pos != string_view::npos) {
                    name_pos += 6;
                    auto name_end = value.find('"', name_pos);
                    if (name_end != string_view::npos) {
                        field.name = value.substr(name_pos, name_end - name_pos);
                    }
                }

                auto fn_pos = value.find("filename=\"");
                if (fn_pos != string_view::npos) {
                    fn_pos += 10;
                    auto fn_end = value.find('"', fn_pos);
                    if (fn_end != string_view::npos) {
                        field.filename = value.substr(fn_pos, fn_end - fn_pos);
                    }
                }
            } else if (key.lowercase() == "content-type") {
                field.content_type = value;
            }

            field.headers[key] = value;
        }
        return true;
    }
} // namespace


string_view multipart_parser::extract_boundary(string_view content_type) {
    constexpr string_view prefix = "boundary=";
    auto pos = content_type.find(prefix);
    if (pos == string_view::npos) {
        return {};
    }
    auto start = pos + prefix.size();
    if (start < content_type.size() && content_type[start] == '"') {
        start++;
        auto end = content_type.find('"', start);
        if (end != string_view::npos) {
            return content_type.substr(start, end - start);
        }
    }

    auto end = content_type.find(';', start);
    if (end == string_view::npos) {
        end = content_type.size();
    }
    while (end > start && content_type[end - 1] == ' ') {
        end--;
    }

    return content_type.substr(start, end - start);
}

vector<multipart_field> multipart_parser::parse(string_view body, string_view boundary) {
    vector<multipart_field> fields;
    if (body.empty() || boundary.empty()) {
        return fields;
    }

    string delim = "--"_s + boundary;
    byte_size total_parsed{0};

    auto pos = body.find(delim.view());
    if (pos == string_view::npos) {
        return fields;
    }

    while (pos != string_view::npos) {
        pos += delim.size();
        if (pos + 2 <= body.size() && body.substr(pos, 2) == "--") {
            break;
        }
        if (pos + 2 <= body.size() && body.substr(pos, 2) == "\r\n") {
            pos += 2;
        }

        auto next = body.find(delim.view(), pos);
        if (next == string_view::npos) {
            break;
        }

        size_t part_end = next;
        if (part_end >= 2 && body.substr(part_end - 2, 2) == "\r\n") {
            part_end -= 2;
        }
        string_view part = body.substr(pos, part_end - pos);

        if (fields.size() >= max_fields) {
            break;
        }

        multipart_field field;
        size_t hdr_offset = 0;
        parse_part_headers(part, hdr_offset, field);

        if (hdr_offset < part.size()) {
            size_t data_len = part.size() - hdr_offset;
            if (data_len > max_field_size.bytes()) {
                fields.clear();
                return fields;
            }
            total_parsed += byte_size{data_len};
            if (total_parsed.bytes() > max_total_size.bytes()) {
                fields.clear();
                return fields;
            }
            field.data = byte_vector(part.data() + hdr_offset, part.data() + part.size());
        }

        if (!field.name.empty()) {
            fields.push_back(move(field));
        }

        pos = next;
    }

    return fields;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
