#include <NeForce/network/http/http_range.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

vector<byte_range> parse_ranges(const string_view range_header, const uint64_t file_size, const size_t max_ranges) {
    vector<byte_range> result;

    if (file_size == 0 || range_header.empty()) {
        return result;
    }

    string_view remaining = range_header;

    if (remaining.starts_with("bytes=")) {
        remaining = remaining.tail(6);
    }

    while (!remaining.empty()) {
        remaining = remaining.trim();

        const size_t dash = remaining.find('-');
        if (dash == string_view::npos) {
            return {};
        }

        const string_view start_str = remaining.view(0, dash).trim();
        const size_t comma = remaining.find(',', dash + 1);
        const size_t end_pos = (comma != string_view::npos) ? comma : remaining.size();
        const string_view end_str = remaining.view(dash + 1, end_pos - dash - 1).trim();

        byte_range range{};

        if (start_str.empty()) {
            if (end_str.empty()) {
                return {};
            }
            uint64_t suffix_len = 0;
            try {
                suffix_len = uinteger64::parse(end_str).value();
            } catch (...) {
                return {};
            }
            if (suffix_len > file_size) {
                range.start = 0;
            } else {
                range.start = file_size - suffix_len;
            }
            range.end = file_size - 1;
        } else {
            try {
                range.start = uinteger64::parse(start_str).value();
            } catch (...) {
                return {};
            }
            if (range.start >= file_size) {
                return {};
            }
            if (end_str.empty()) {
                range.end = file_size - 1;
            } else {
                try {
                    range.end = uinteger64::parse(end_str).value();
                } catch (...) {
                    return {};
                }
                if (range.end >= file_size) {
                    range.end = file_size - 1;
                }
            }
        }

        if (range.start > range.end) {
            return {};
        }
        if (result.size() >= max_ranges) {
            return {};
        }

        result.push_back(range);

        if (comma == string_view::npos) {
            break;
        }
        remaining = remaining.tail(comma + 1);
    }

    return result;
}

string build_content_range(const byte_range& range, const uint64_t total_size) {
    string result;
    result += "bytes ";
    result += to_string(range.start);
    result += '-';
    result += to_string(range.end);
    result += '/';
    result += to_string(total_size);
    return result;
}

string build_multipart_ranges(const vector<byte_range>& ranges, const string_view content_type,
                              const string_view boundary, function<string(const byte_range&)> get_range_body,
                              const uint64_t total_size) {
    string result;
    result += "--";
    result += boundary;
    for (const auto& r: ranges) {
        result += "\r\nContent-Type: ";
        result += content_type;
        result += "\r\nContent-Range: ";
        result += build_content_range(r, total_size);
        result += "\r\n\r\n";
        result += get_range_body(r);
        result += "\r\n--";
        result += boundary;
    }
    result += "--\r\n";
    return result;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
