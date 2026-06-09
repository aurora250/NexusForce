#include <NeForce/network/http/chunked_reader.hpp>
#include <NeForce/network/http/http_constants.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    size_t parse_chunk_size(string_view line) {
        auto semi = line.find(';');
        if (semi != string_view::npos) {
            line = line.substr(0, semi);
        }

        while (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
            line = line.substr(1);
        }

        if (line.empty()) {
            NEFORCE_THROW_EXCEPTION(http_exception("Empty chunk size"));
        }

        return hexadecimal::parse(line).value();
    }
} // namespace


bool chunked_body_reader::read_line(string& line) {
    line.clear();
    char c = '\0';
    while (true) {
        auto n = socket_->receive({&c, 1}, 1);
        if (n != 1) {
            return false;
        }
        if (c == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return true;
        }
        line += c;
        if (line.size() > 4096) {
            NEFORCE_THROW_EXCEPTION(http_exception("Chunk header line too long"));
        }
    }
}

void chunked_body_reader::parse_trailers() {
    if (trailers_parsed_) {
        return;
    }
    trailers_parsed_ = true;

    string line;
    while (read_line(line)) {
        if (line.empty()) {
            break;
        }
        auto colon = line.find(':');
        if (colon != string::npos) {
            string_view key = line.view(0, colon);
            string_view value = line.view(colon + 1);
            while (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            trailers_[key] = value;
        }
    }
}

bool chunked_body_reader::next_chunk(byte_vector& out) {
    if (complete_) {
        return false;
    }

    string size_line;
    if (!read_line(size_line)) {
        NEFORCE_THROW_EXCEPTION(http_exception("Failed to read chunk size"));
    }

    size_t chunk_size = parse_chunk_size(size_line.view());

    if (chunk_size == 0) {
        parse_trailers();
        complete_ = true;
        return false;
    }

    if (chunk_size > max_chunk_size.bytes()) {
        NEFORCE_THROW_EXCEPTION(http_exception("Chunk exceeds max_chunk_size"));
    }

    total_read_ += byte_size{chunk_size};
    if (total_read_.bytes() > max_total_size.bytes()) {
        NEFORCE_THROW_EXCEPTION(http_exception("Total chunked body exceeds max_total_size"));
    }

    out.resize(chunk_size);
    ssize_t total_received = 0;
    while (static_cast<size_t>(total_received) < chunk_size) {
        ssize_t n = socket_->receive(memory_view<char>{reinterpret_cast<char*>(out.data()) + total_received,
                                                       chunk_size - static_cast<size_t>(total_received)});
        if (n <= 0) {
            NEFORCE_THROW_EXCEPTION(http_exception("Failed to read complete chunk data"));
        }
        total_received += n;
    }

    string crlf;
    if (!read_line(crlf)) {
        NEFORCE_THROW_EXCEPTION(http_exception("Missing CRLF after chunk data"));
    }

    return true;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
