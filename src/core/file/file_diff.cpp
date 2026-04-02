#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/file_diff.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr size_t buffer_size = 8192;
}


bool file_diff::compare(const path& file1, const path& file2, const bool binary) {
    return binary ? compare_binary(file1, file2) : compare_text(file1, file2);
}

bool file_diff::compare_binary(const path& file1, const path& file2) {
    file f1, f2;

    if (!f1.open(file1, false, file_access::READ, file_shared::SHARE_READ)) {
        return false;
    }
    if (!f2.open(file2, false, file_access::READ, file_shared::SHARE_READ)) {
        return false;
    }

    const uint64_t size1 = f1.size64();
    const uint64_t size2 = f2.size64();
    if (size1 != size2) {
        return false;
    }
    if (size1 == 0) {
        return true;
    }

    string buf1(buffer_size, '\0');
    string buf2(buffer_size, '\0');
    size_type remaining = size1;

    while (remaining > 0) {
        const size_type chunk = min(remaining, static_cast<size_type>(buffer_size));
        const size_type r1 = f1.read_binary(buf1, chunk);
        const size_type r2 = f2.read_binary(buf2, chunk);
        if (r1 != chunk || r2 != chunk) {
            return false;
        }
        if (memory_compare(buf1.data(), buf2.data(), chunk) != 0) {
            return false;
        }
        remaining -= chunk;
    }
    return true;
}

bool file_diff::compare_text(const path& file1, const path& file2, const bool ignore_case,
                             const bool ignore_whitespace) {
    if (!ignore_case && !ignore_whitespace) {
        return compare_binary(file1, file2);
    }

    file f1, f2;

    if (!f1.open(file1, false, file_access::READ, file_shared::SHARE_READ)) {
        return false;
    }
    if (!f2.open(file2, false, file_access::READ, file_shared::SHARE_READ)) {
        return false;
    }

    string content1, content2;
    {
        const size_type sz1 = f1.info().size();
        const size_type sz2 = f2.info().size();
        content1.resize(sz1);
        content2.resize(sz2);
        if (f1.read_binary(content1, sz1) != sz1) {
            return false;
        }
        if (f2.read_binary(content2, sz2) != sz2) {
            return false;
        }
    }

    auto split_lines = [](const string& content, vector<string>& lines) {
        if (content.empty()) {
            return;
        }
        size_t start = 0;

        while (start < content.size()) {
            const size_t end = content.find('\n', start);
            if (end == string::npos) {
                string line = content.substr(start);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                lines.emplace_back(move(line));
                break;
            }

            string line = content.substr(start, end - start);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.emplace_back(move(line));
            start = end + 1;
        }
    };

    vector<string> lines1, lines2;
    split_lines(content1, lines1);
    split_lines(content2, lines2);

    if (lines1.size() != lines2.size()) {
        return false;
    }

    auto normalize = [ignore_case, ignore_whitespace](string& s) {
        if (ignore_whitespace) {
            size_t lo = 0, hi = s.size();
            while (lo < hi && is_space(s[lo])) {
                ++lo;
            }
            while (hi > lo && is_space(s[hi - 1])) {
                --hi;
            }

            string result;
            result.reserve(hi - lo);
            bool in_space = false;
            for (size_t i = lo; i < hi; ++i) {
                if (is_space(s[i])) {
                    if (!in_space) {
                        result += ' ';
                        in_space = true;
                    }
                } else {
                    result += s[i];
                    in_space = false;
                }
            }
            s = move(result);
        }
        if (ignore_case) {
            s.lowercase();
        }
    };

    for (size_t i = 0; i < lines1.size(); ++i) {
        normalize(lines1[i]);
        normalize(lines2[i]);
        if (lines1[i] != lines2[i]) {
            return false;
        }
    }
    return true;
}

vector<file_diff::binary_diff_entry> file_diff::binary_diff(const path& file1, const path& file2,
                                                            const size_type max_diffs) {
    vector<binary_diff_entry> diffs;
    diffs.reserve(min(max_diffs, static_cast<size_type>(256)));

    file f1, f2;

    if (!f1.open(file1, false, file_access::READ, file_shared::SHARE_READ)) {
        return diffs;
    }
    if (!f2.open(file2, false, file_access::READ, file_shared::SHARE_READ)) {
        return diffs;
    }

    const size_type size1 = f1.info().size();
    const size_type size2 = f2.info().size();

    if (size1 != size2 && diffs.size() < max_diffs) {
        binary_diff_entry e{};
        e.offset = static_cast<difference_type>(min(size1, size2));
        e.is_size_diff = true;
        e.size_diff = static_cast<int64_t>(size1) - static_cast<int64_t>(size2);
        diffs.push_back(e);
    }

    const size_type min_size = min(size1, size2);
    if (min_size == 0) {
        return diffs;
    }

    string buf1(buffer_size, '\0');
    string buf2(buffer_size, '\0');
    difference_type offset = 0;

    while (static_cast<size_type>(offset) < min_size && diffs.size() < max_diffs) {
        const size_type chunk = min(min_size - static_cast<size_type>(offset), static_cast<size_type>(buffer_size));

        const size_type r1 = f1.read_binary(buf1, chunk);
        const size_type r2 = f2.read_binary(buf2, chunk);
        if (r1 != chunk || r2 != chunk) {
            break;
        }

        if (memory_compare(buf1.data(), buf2.data(), chunk) != 0) {
            for (size_type i = 0; i < chunk && diffs.size() < max_diffs; ++i) {
                if (buf1[i] != buf2[i]) {
                    binary_diff_entry e{};
                    e.offset = offset + static_cast<difference_type>(i);
                    e.byte1 = static_cast<byte_t>(buf1[i]);
                    e.byte2 = static_cast<byte_t>(buf2[i]);
                    diffs.push_back(e);
                }
            }
        }
        offset += static_cast<difference_type>(chunk);
    }
    return diffs;
}

NEFORCE_END_NAMESPACE__
