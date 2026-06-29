#include <NeForce/core/memory/byte_cursor.hpp>
#include <NeForce/network/http/http2_protocol.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    struct huffman_code {
        uint32_t code;
        uint8_t bits;
    };

    constexpr huffman_code HUFFMAN_TABLE[256] = {
            {0x1ff8, 13},    {0x7fffd8, 23},   {0xfffffe2, 28}, {0xfffffe3, 28}, {0xfffffe4, 28},  {0xfffffe5, 28},
            {0xfffffe6, 28}, {0xfffffe7, 28},  {0xfffffe8, 28}, {0xffffea, 24},  {0x3ffffffc, 30}, {0xfffffe9, 28},
            {0xfffffea, 28}, {0x3ffffffd, 30}, {0xfffffeb, 28}, {0xfffffec, 28}, {0xfffffed, 28},  {0xfffffee, 28},
            {0xfffffef, 28}, {0xffffff0, 28},  {0xffffff1, 28}, {0xffffff2, 28}, {0x3ffffffe, 30}, {0xffffff3, 28},
            {0xffffff4, 28}, {0xffffff5, 28},  {0xffffff6, 28}, {0xffffff7, 28}, {0xffffff8, 28},  {0xffffff9, 28},
            {0xffffffa, 28}, {0xffffffb, 28},  {0x14, 6},       {0x3f8, 10},     {0x3f9, 10},      {0xffa, 12},
            {0x1ff9, 13},    {0x15, 6},        {0xf8, 8},       {0x7fa, 11},     {0x3fa, 10},      {0x3fb, 10},
            {0xf9, 8},       {0x7fb, 11},      {0xfa, 8},       {0x16, 6},       {0x17, 6},        {0x18, 6},
            {0x0, 5},        {0x1, 5},         {0x2, 5},        {0x19, 6},       {0x1a, 6},        {0x1b, 6},
            {0x1c, 6},       {0x1d, 6},        {0x1e, 6},       {0x1f, 6},       {0x5c, 7},        {0xfb, 8},
            {0x7ffc, 15},    {0x20, 6},        {0xffb, 12},     {0x3fc, 10},     {0x1ffa, 13},     {0x21, 6},
            {0x5d, 7},       {0x5e, 7},        {0x5f, 7},       {0x60, 7},       {0x61, 7},        {0x62, 7},
            {0x63, 7},       {0x64, 7},        {0x65, 7},       {0x66, 7},       {0x67, 7},        {0x68, 7},
            {0x69, 7},       {0x6a, 7},        {0x6b, 7},       {0x6c, 7},       {0x6d, 7},        {0x6e, 7},
            {0x6f, 7},       {0x70, 7},        {0x71, 7},       {0x72, 7},       {0xfc, 8},        {0x73, 7},
            {0xfd, 8},       {0x1ffb, 13},     {0x7fff0, 19},   {0x1ffc, 13},    {0x3ffc, 14},     {0x22, 6},
            {0x7ffd, 15},    {0x3, 5},         {0x23, 6},       {0x4, 5},        {0x24, 6},        {0x5, 5},
            {0x25, 6},       {0x26, 6},        {0x27, 6},       {0x6, 5},        {0x74, 7},        {0x75, 7},
            {0x28, 6},       {0x29, 6},        {0x2a, 6},       {0x7, 5},        {0x2b, 6},        {0x76, 7},
            {0x2c, 6},       {0x8, 5},         {0x9, 5},        {0x2d, 6},       {0x77, 7},        {0x78, 7},
            {0x79, 7},       {0x7a, 7},        {0x7b, 7},       {0x7ffe, 15},    {0x7fc, 11},      {0x3ffd, 14},
            {0x1ffd, 13},    {0xffffffc, 28},  {0xfffe6, 20},   {0x3fffd2, 22},  {0xfffe7, 20},    {0xfffe8, 20},
            {0x3fffd3, 22},  {0x3fffd4, 22},   {0x3fffd5, 22},  {0x7fffd9, 23},  {0x3fffd6, 22},   {0x7fffda, 23},
            {0x7fffdb, 23},  {0x7fffdc, 23},   {0x7fffdd, 23},  {0x7fffde, 23},  {0xffffeb, 24},   {0x7fffdf, 23},
            {0xffffec, 24},  {0xffffed, 24},   {0x3fffd7, 22},  {0x7fffe0, 23},  {0xffffee, 24},   {0x7fffe1, 23},
            {0x7fffe2, 23},  {0x7fffe3, 23},   {0x7fffe4, 23},  {0x1fffdc, 21},  {0x3fffd8, 22},   {0x7fffe5, 23},
            {0x3fffd9, 22},  {0x7fffe6, 23},   {0x7fffe7, 23},  {0xffffef, 24},  {0x3fffda, 22},   {0x1fffdd, 21},
            {0xfffe9, 20},   {0x3fffdb, 22},   {0x3fffdc, 22},  {0x7fffe8, 23},  {0x7fffe9, 23},   {0x1fffde, 21},
            {0x7fffea, 23},  {0x3fffdd, 22},   {0x3fffde, 22},  {0xfffff0, 24},  {0x1fffdf, 21},   {0x3fffdf, 22},
            {0x7fffeb, 23},  {0x7fffec, 23},   {0x1fffe0, 21},  {0x1fffe1, 21},  {0x3fffe0, 22},   {0x1fffe2, 21},
            {0x7fffed, 23},  {0x3fffe1, 22},   {0x7fffee, 23},  {0x7fffef, 23},  {0xfffea, 20},    {0x3fffe2, 22},
            {0x3fffe3, 22},  {0x3fffe4, 22},   {0x7ffff0, 23},  {0x3fffe5, 22},  {0x3fffe6, 22},   {0x7ffff1, 23},
            {0x3ffffe0, 26}, {0x3ffffe1, 26},  {0xfffeb, 20},   {0x7fff1, 19},   {0x3fffe7, 22},   {0x7ffff2, 23},
            {0x3fffe8, 22},  {0x1ffffec, 25},  {0x3ffffe2, 26}, {0x3ffffe3, 26}, {0x3ffffe4, 26},  {0x7ffffde, 27},
            {0x7ffffdf, 27}, {0x3ffffe5, 26},  {0xfffff1, 24},  {0x1ffffed, 25}, {0x7fff2, 19},    {0x1fffe3, 21},
            {0x3ffffe6, 26}, {0x7ffffe0, 27},  {0x7ffffe1, 27}, {0x3ffffe7, 26}, {0x7ffffe2, 27},  {0xfffff2, 24},
            {0x1fffe4, 21},  {0x1fffe5, 21},   {0x3ffffe8, 26}, {0x3ffffe9, 26}, {0xffffffd, 28},  {0x7ffffe3, 27},
            {0x7ffffe4, 27}, {0x7ffffe5, 27},  {0xfffec, 20},   {0xfffff3, 24},  {0xfffed, 20},    {0x1fffe6, 21},
            {0x3fffe9, 22},  {0x1fffe7, 21},   {0x1fffe8, 21},  {0x7ffff3, 23},  {0x3fffea, 22},   {0x3fffeb, 22},
            {0x1ffffee, 25}, {0x1ffffef, 25},  {0xfffff4, 24},  {0xfffff5, 24},  {0x3ffffea, 26},  {0x7ffff4, 23},
            {0x3ffffeb, 26}, {0x7ffffe6, 27},  {0x3ffffec, 26}, {0x3ffffed, 26}, {0x7ffffe7, 27},  {0x7ffffe8, 27},
            {0x7ffffe9, 27}, {0x7ffffea, 27},  {0x7ffffeb, 27}, {0xffffffe, 28}, {0x7ffffec, 27},  {0x7ffffed, 27},
            {0x7ffffee, 27}, {0x7ffffef, 27},  {0x7fffff0, 27}, {0x3ffffee, 26},
    };

    decltype(auto) HPACK_STATIC_TABLE() {
        static const hpack_header_field HPACK_STATIC_TABLE[HPACK_STATIC_TABLE_SIZE] = {
                {":authority", ""},
                {":method", "GET"},
                {":method", "POST"},
                {":path", "/"},
                {":path", "/index.html"},
                {":scheme", "http"},
                {":scheme", "https"},
                {":status", "200"},
                {":status", "204"},
                {":status", "206"},
                {":status", "304"},
                {":status", "400"},
                {":status", "404"},
                {":status", "500"},
                {"accept-charset", ""},
                {"accept-encoding", "gzip, deflate"},
                {"accept-language", ""},
                {"accept-ranges", ""},
                {"accept", ""},
                {"access-control-allow-origin", ""},
                {"age", ""},
                {"allow", ""},
                {"authorization", ""},
                {"cache-control", ""},
                {"content-disposition", ""},
                {"content-encoding", ""},
                {"content-language", ""},
                {"content-length", ""},
                {"content-location", ""},
                {"content-range", ""},
                {"content-type", ""},
                {"cookie", ""},
                {"date", ""},
                {"etag", ""},
                {"expect", ""},
                {"expires", ""},
                {"from", ""},
                {"host", ""},
                {"if-match", ""},
                {"if-modified-since", ""},
                {"if-none-match", ""},
                {"if-range", ""},
                {"if-unmodified-since", ""},
                {"last-modified", ""},
                {"link", ""},
                {"location", ""},
                {"max-forwards", ""},
                {"proxy-authenticate", ""},
                {"proxy-authorization", ""},
                {"range", ""},
                {"referer", ""},
                {"refresh", ""},
                {"retry-after", ""},
                {"server", ""},
                {"set-cookie", ""},
                {"strict-transport-security", ""},
                {"transfer-encoding", ""},
                {"user-agent", ""},
                {"vary", ""},
                {"via", ""},
                {"www-authenticate", ""},
        };
        return (HPACK_STATIC_TABLE);
    }

    void encode_integer(uint32_t value, uint8_t prefix_bits, byte_vector& output) {
        const uint8_t max_prefix = (1U << prefix_bits) - 1;
        if (value < max_prefix) {
            output.back() |= static_cast<uint8_t>(value);
            return;
        }
        output.back() |= max_prefix;
        value -= max_prefix;
        while (value >= 128) {
            output.push_back(static_cast<byte_t>(value % 128 + 128));
            value /= 128;
        }
        output.push_back(static_cast<byte_t>(value));
    }

    void encode_huffman(const byte_t* data, size_t len, byte_vector& output) {
        size_t total_bits = 0;
        for (size_t i = 0; i < len; ++i) {
            total_bits += HUFFMAN_TABLE[data[i]].bits;
        }

        const size_t hdr_pos = output.size();
        output.push_back(1);
        encode_integer(static_cast<uint32_t>(total_bits / 8 + (((total_bits % 8) != 0U) ? 1 : 0)), 7, output);

        output.resize(hdr_pos);
        auto encoded_len = static_cast<uint32_t>((total_bits + 7) / 8);
        output.push_back(0x80);
        encode_integer(encoded_len, 7, output);

        uint64_t bit_buf = 0;
        size_t bits_in_buf = 0;
        for (size_t i = 0; i < len; ++i) {
            const auto& hc = HUFFMAN_TABLE[data[i]];
            bit_buf = (bit_buf << hc.bits) | hc.code;
            bits_in_buf += hc.bits;

            while (bits_in_buf >= 8) {
                bits_in_buf -= 8;
                output.push_back(static_cast<byte_t>((bit_buf >> bits_in_buf) & 0xFF));
            }
        }

        if (bits_in_buf > 0) {
            const uint8_t padding = (1U << (8 - bits_in_buf)) - 1;
            output.push_back(static_cast<byte_t>(((bit_buf << (8 - bits_in_buf)) & 0xFF) | padding));
        }
    }

    void encode_string(const string& str, byte_vector& output) {
        encode_huffman(reinterpret_cast<const byte_t*>(str.data()), str.size(), output);
    }

    uint32_t decode_integer(byte_cursor& cur, uint8_t prefix_bits) {
        auto first_opt = cur.try_read_byte();
        if (!first_opt) {
            return 0;
        }
        const uint8_t max_prefix = (1U << prefix_bits) - 1;
        uint8_t first = *first_opt & max_prefix;

        if (first < max_prefix) {
            return first;
        }

        uint32_t value = max_prefix;
        uint32_t m = 0;
        constexpr int max_octets = 10;
        for (int i = 0; i < max_octets; ++i) {
            auto b_opt = cur.try_read_byte();
            if (!b_opt) {
                break;
            }
            const uint8_t b = *b_opt;
            value += (b & 0x7F) << m;
            m += 7;
            if ((b & 0x80) == 0) {
                return value;
            }
        }
        return value;
    }

    string decode_huffman(cbyte_view input) {
        static constexpr uint64_t EOS_CODE = 0x3fffffff;
        static constexpr uint8_t EOS_BITS = 30;

        byte_cursor cur(input);
        string result;
        result.reserve(input.size() * 2);

        while (!cur.exhausted() || cur.bits_remaining() >= 5) {
            while (cur.bits_remaining() < 32 && !cur.exhausted()) {
                cur.refill_bits();
            }

            if (cur.bits_remaining() == 0) {
                break;
            }

            auto eos = cur.try_peek_bits(EOS_BITS);
            if (eos && *eos == EOS_CODE) {
                return {};
            }

            bool matched = false;
            for (size_t c = 0; c < 256; ++c) {
                const auto& hc = HUFFMAN_TABLE[c];
                if (hc.bits > cur.bits_remaining()) {
                    continue;
                }
                auto peeked = cur.try_peek_bits(hc.bits);
                if (peeked && *peeked == hc.code) {
                    result += static_cast<char>(c);
                    cur.skip_bits(hc.bits);
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                break;
            }
        }

        size_t remaining = cur.bits_remaining();
        if (remaining > 7) {
            return {};
        }
        if (remaining > 0) {
            auto padding = cur.try_read_bits(static_cast<uint8_t>(remaining));
            if (!padding) {
                return {};
            }
            const uint64_t mask = (1ULL << remaining) - 1;
            if ((*padding & mask) != mask) {
                return {};
            }
        }

        return result;
    }

    string decode_string(byte_cursor& cur) {
        auto first = cur.peek_byte();
        if (!first) {
            return {};
        }
        bool huffman = (*first & 0x80) != 0;

        uint32_t len = decode_integer(cur, 7);

        auto bytes = cur.try_read_bytes(len);
        if (!bytes) {
            return {};
        }

        if (huffman) {
            return decode_huffman(*bytes);
        }
        const auto* raw = bytes->data();
        return {reinterpret_cast<const char*>(raw), len};
    }


    byte_vector build_frame_header(http2_frame_type type, uint8_t flags, uint32_t stream_id, size_t payload_len) {
        byte_vector hdr(9);
        hdr[0] = static_cast<byte_t>((payload_len >> 16) & 0xFF);
        hdr[1] = static_cast<byte_t>((payload_len >> 8) & 0xFF);
        hdr[2] = static_cast<byte_t>(payload_len & 0xFF);
        hdr[3] = static_cast<uint8_t>(type);
        hdr[4] = flags;
        hdr[5] = static_cast<byte_t>((stream_id >> 24) & 0x7F);
        hdr[6] = static_cast<byte_t>((stream_id >> 16) & 0xFF);
        hdr[7] = static_cast<byte_t>((stream_id >> 8) & 0xFF);
        hdr[8] = static_cast<byte_t>(stream_id & 0xFF);
        return hdr;
    }
} // namespace


hpack_encoder::hpack_encoder(uint32_t max_table_size) :
max_table_size_(max_table_size) {}

void hpack_encoder::set_max_table_size(uint32_t size) {
    max_table_size_ = size;
    evict_for_size(0);
}

void hpack_encoder::add_to_dynamic_table(const string& name, const string& value) {
    const size_t entry_size = name.size() + value.size() + 32;
    evict_for_size(entry_size);

    if (entry_size > max_table_size_) {
        return;
    }

    table_entry entry;
    entry.name = name;
    entry.value = value;
    entry.name_len = name.size();
    entry.value_len = value.size();
    dynamic_table_.insert(dynamic_table_.begin(), move(entry));
    current_table_size_ += entry_size;
}

void hpack_encoder::evict_for_size(uint32_t needed) {
    while (current_table_size_ + needed > max_table_size_ && !dynamic_table_.empty()) {
        const auto& last = dynamic_table_.back();
        current_table_size_ -= (last.name_len + last.value_len + 32);
        dynamic_table_.pop_back();
    }
}

int32_t hpack_encoder::find_in_tables(const string& name, const string& value) const {
    for (size_t i = 0; i < HPACK_STATIC_TABLE_SIZE; ++i) {
        if (HPACK_STATIC_TABLE()[i].name == name && HPACK_STATIC_TABLE()[i].value == value) {
            return static_cast<int32_t>(i + 1);
        }
    }
    for (size_t i = 0; i < dynamic_table_.size(); ++i) {
        if (dynamic_table_[i].name == name && dynamic_table_[i].value == value) {
            return static_cast<int32_t>(HPACK_STATIC_TABLE_SIZE + i + 1);
        }
    }
    return -1;
}

int32_t hpack_encoder::find_name_in_tables(const string& name) const {
    for (size_t i = 0; i < HPACK_STATIC_TABLE_SIZE; ++i) {
        if (HPACK_STATIC_TABLE()[i].name == name) {
            return static_cast<int32_t>(i + 1);
        }
    }
    for (size_t i = 0; i < dynamic_table_.size(); ++i) {
        if (dynamic_table_[i].name == name) {
            return static_cast<int32_t>(HPACK_STATIC_TABLE_SIZE + i + 1);
        }
    }
    return -1;
}

byte_vector hpack_encoder::encode(const vector<hpack_header_field>& headers) {
    byte_vector output;
    output.reserve(256);

    for (const auto& hdr: headers) {
        const int32_t idx = find_in_tables(hdr.name, hdr.value);
        if (idx >= 1 && idx <= HPACK_STATIC_TABLE_SIZE) {
            output.push_back(0x80);
            encode_integer(static_cast<uint32_t>(idx), 7, output);
            continue;
        }
        if (idx >= 62) {
            output.push_back(0x80);
            encode_integer(static_cast<uint32_t>(idx), 7, output);
            continue;
        }

        const int32_t name_idx = find_name_in_tables(hdr.name);
        if (name_idx >= 1) {
            output.push_back(0x40);
            encode_integer(static_cast<uint32_t>(name_idx), 6, output);
        } else {
            output.push_back(0x40);
            encode_integer(0, 6, output);
            encode_string(hdr.name, output);
        }
        encode_string(hdr.value, output);
        add_to_dynamic_table(hdr.name, hdr.value);
    }

    return output;
}

hpack_decoder::hpack_decoder(uint32_t max_table_size) :
max_table_size_(max_table_size) {}

void hpack_decoder::set_max_table_size(uint32_t size) {
    max_table_size_ = size;
    evict_for_size(0);
}

void hpack_decoder::add_to_dynamic_table(const string& name, const string& value) {
    const size_t entry_size = name.size() + value.size() + 32;
    evict_for_size(entry_size);
    if (entry_size > max_table_size_) {
        return;
    }
    table_entry entry;
    entry.name = name;
    entry.value = value;
    entry.name_len = name.size();
    entry.value_len = value.size();
    dynamic_table_.insert(dynamic_table_.begin(), move(entry));
    current_table_size_ += entry_size;
}

void hpack_decoder::evict_for_size(uint32_t needed) {
    while (current_table_size_ + needed > max_table_size_ && !dynamic_table_.empty()) {
        const auto& last = dynamic_table_.back();
        current_table_size_ -= (last.name_len + last.value_len + 32);
        dynamic_table_.pop_back();
    }
}

vector<hpack_header_field> hpack_decoder::decode(const byte_t* data, size_t len) {
    vector<hpack_header_field> result;
    byte_cursor cur(data, len);
    decode_incremental(cur, [&result](const string& name, const string& value) { result.push_back({name, value}); });
    return result;
}

void hpack_decoder::decode_incremental(byte_cursor& cur, header_callback cb) {
    while (!cur.exhausted()) {
        auto first_opt = cur.peek_byte();
        if (!first_opt) {
            break;
        }
        const uint8_t first = *first_opt;

        if ((first & 0x80) != 0) {
            uint32_t idx = decode_integer(cur, 7);
            if (idx == 0) {
                continue;
            }
            if (idx <= HPACK_STATIC_TABLE_SIZE) {
                const auto* e = &HPACK_STATIC_TABLE()[idx - 1];
                cb(e->name, e->value);
            } else {
                const size_t dynamic_idx = idx - HPACK_STATIC_TABLE_SIZE - 1;
                if (dynamic_idx < dynamic_table_.size()) {
                    cb(dynamic_table_[dynamic_idx].name, dynamic_table_[dynamic_idx].value);
                }
            }
        } else if ((first & 0x40) != 0) {
            uint32_t name_idx = decode_integer(cur, 6);
            string name;
            if (name_idx == 0) {
                name = decode_string(cur);
            } else if (name_idx <= HPACK_STATIC_TABLE_SIZE) {
                name = HPACK_STATIC_TABLE()[name_idx - 1].name;
            } else {
                const size_t dynamic_idx = name_idx - HPACK_STATIC_TABLE_SIZE - 1;
                if (dynamic_idx < dynamic_table_.size()) {
                    name = dynamic_table_[dynamic_idx].name;
                }
            }
            string value = decode_string(cur);
            add_to_dynamic_table(name, value);
            cb(name, value);
        } else if ((first & 0x20) != 0) {
            uint32_t new_size = decode_integer(cur, 5);
            set_max_table_size(new_size);
        } else if ((first & 0xF0) == 0x00) {
            uint32_t name_idx = decode_integer(cur, 4);
            string name;
            if (name_idx == 0) {
                name = decode_string(cur);
            } else if (name_idx <= HPACK_STATIC_TABLE_SIZE) {
                name = HPACK_STATIC_TABLE()[name_idx - 1].name;
            } else {
                const size_t dynamic_idx = name_idx - HPACK_STATIC_TABLE_SIZE - 1;
                if (dynamic_idx < dynamic_table_.size()) {
                    name = dynamic_table_[dynamic_idx].name;
                }
            }
            string value = decode_string(cur);
            cb(name, value);
        } else if ((first & 0xF0) == 0x10) {
            uint32_t name_idx = decode_integer(cur, 4);
            string name;
            if (name_idx == 0) {
                name = decode_string(cur);
            } else if (name_idx <= HPACK_STATIC_TABLE_SIZE) {
                name = HPACK_STATIC_TABLE()[name_idx - 1].name;
            } else {
                const size_t dynamic_idx = name_idx - HPACK_STATIC_TABLE_SIZE - 1;
                if (dynamic_idx < dynamic_table_.size()) {
                    name = dynamic_table_[dynamic_idx].name;
                }
            }
            string value = decode_string(cur);
            cb(name, value);
        }
    }
}

byte_vector http2_framer::encode_data_frame(const http2_data_frame& frame) {
    size_t payload_len = frame.data.size();
    uint8_t flags = 0;
    if (frame.end_stream) {
        flags |= HTTP2_FLAG_END_STREAM;
    }
    if (frame.pad_length > 0) {
        flags |= HTTP2_FLAG_PADDED;
        payload_len += 1 + frame.pad_length;
    }

    byte_vector result = build_frame_header(http2_frame_type::DATA, flags, frame.stream_id, payload_len);
    if (frame.pad_length > 0) {
        result.push_back(frame.pad_length);
    }
    result.insert(result.end(), frame.data.begin(), frame.data.end());
    if (frame.pad_length > 0) {
        result.resize(result.size() + frame.pad_length, 0);
    }
    return result;
}

byte_vector http2_framer::encode_headers_frame(const http2_headers_frame& frame) {
    size_t payload_len = frame.header_block.size();
    uint8_t flags = 0;
    if (frame.end_stream) {
        flags |= HTTP2_FLAG_END_STREAM;
    }
    if (frame.end_headers) {
        flags |= HTTP2_FLAG_END_HEADERS;
    }
    if (frame.has_priority) {
        flags |= HTTP2_FLAG_PRIORITY;
        payload_len += 5;
    }
    if (frame.pad_length > 0) {
        flags |= HTTP2_FLAG_PADDED;
        payload_len += 1 + frame.pad_length;
    }

    byte_vector result = build_frame_header(http2_frame_type::HEADERS, flags, frame.stream_id, payload_len);
    if (frame.pad_length > 0) {
        result.push_back(frame.pad_length);
    }
    if (frame.has_priority) {
        result.push_back(static_cast<byte_t>((frame.priority.stream_dependency >> 24) & 0x7F));
        result.push_back(static_cast<byte_t>((frame.priority.stream_dependency >> 16) & 0xFF));
        result.push_back(static_cast<byte_t>((frame.priority.stream_dependency >> 8) & 0xFF));
        result.push_back(static_cast<byte_t>(frame.priority.stream_dependency & 0xFF));
        result.push_back(frame.priority.weight);
    }
    result.insert(result.end(), frame.header_block.begin(), frame.header_block.end());
    if (frame.pad_length > 0) {
        result.resize(result.size() + frame.pad_length, 0);
    }
    return result;
}

byte_vector http2_framer::encode_push_promise_frame(const http2_push_promise_frame& frame) {
    size_t payload_len = 4 + frame.header_block.size(); // Promised Stream ID + header block
    uint8_t flags = 0;
    if (frame.end_headers) {
        flags |= HTTP2_FLAG_END_HEADERS;
    }
    if (frame.pad_length > 0) {
        flags |= HTTP2_FLAG_PADDED;
        payload_len += 1 + frame.pad_length;
    }

    byte_vector result = build_frame_header(http2_frame_type::PUSH_PROMISE, flags, frame.stream_id, payload_len);
    if (frame.pad_length > 0) {
        result.push_back(frame.pad_length);
    }
    // Promised Stream ID (31-bit)
    uint32_t psid = frame.promised_stream_id & 0x7FFFFFFF;
    result.push_back(static_cast<byte_t>((psid >> 24) & 0xFF));
    result.push_back(static_cast<byte_t>((psid >> 16) & 0xFF));
    result.push_back(static_cast<byte_t>((psid >> 8) & 0xFF));
    result.push_back(static_cast<byte_t>(psid & 0xFF));
    // Header block fragment
    result.insert(result.end(), frame.header_block.begin(), frame.header_block.end());
    if (frame.pad_length > 0) {
        result.resize(result.size() + frame.pad_length, 0);
    }
    return result;
}

byte_vector http2_framer::encode_rst_stream_frame(const http2_rst_stream_frame& frame) {
    byte_vector result = build_frame_header(http2_frame_type::RST_STREAM, 0, frame.stream_id, 4);
    auto code = static_cast<uint32_t>(frame.error_code);
    result.push_back(static_cast<byte_t>((code >> 24) & 0xFF));
    result.push_back(static_cast<byte_t>((code >> 16) & 0xFF));
    result.push_back(static_cast<byte_t>((code >> 8) & 0xFF));
    result.push_back(static_cast<byte_t>(code & 0xFF));
    return result;
}

byte_vector http2_framer::encode_settings_frame(const http2_settings_frame& frame) {
    uint8_t flags = frame.ack ? HTTP2_FLAG_ACK : 0;
    size_t payload_len = frame.entries.size() * 6;

    byte_vector result = build_frame_header(http2_frame_type::SETTINGS, flags, 0, payload_len);
    for (const auto& entry: frame.entries) {
        auto id = static_cast<uint16_t>(entry.id);
        result.push_back(static_cast<byte_t>((id >> 8) & 0xFF));
        result.push_back(static_cast<byte_t>(id & 0xFF));
        result.push_back(static_cast<byte_t>((entry.value >> 24) & 0xFF));
        result.push_back(static_cast<byte_t>((entry.value >> 16) & 0xFF));
        result.push_back(static_cast<byte_t>((entry.value >> 8) & 0xFF));
        result.push_back(static_cast<byte_t>(entry.value & 0xFF));
    }
    return result;
}

byte_vector http2_framer::encode_ping_frame(const http2_ping_frame& frame) {
    uint8_t flags = frame.ack ? HTTP2_FLAG_ACK : 0;
    byte_vector result = build_frame_header(http2_frame_type::PING, flags, 0, 8);
    const uint64_t data = frame.opaque_data;
    for (int i = 7; i >= 0; --i) {
        result.push_back(static_cast<byte_t>((data >> (i * 8)) & 0xFF));
    }
    return result;
}

byte_vector http2_framer::encode_goaway_frame(const http2_goaway_frame& frame) {
    size_t payload_len = 8 + frame.debug_data.size();
    byte_vector result = build_frame_header(http2_frame_type::GOAWAY, 0, 0, payload_len);

    uint32_t id = frame.last_stream_id;
    result.push_back(static_cast<byte_t>((id >> 24) & 0x7F));
    result.push_back(static_cast<byte_t>((id >> 16) & 0xFF));
    result.push_back(static_cast<byte_t>((id >> 8) & 0xFF));
    result.push_back(static_cast<byte_t>(id & 0xFF));

    auto code = static_cast<uint32_t>(frame.error_code);
    result.push_back(static_cast<byte_t>((code >> 24) & 0xFF));
    result.push_back(static_cast<byte_t>((code >> 16) & 0xFF));
    result.push_back(static_cast<byte_t>((code >> 8) & 0xFF));
    result.push_back(static_cast<byte_t>(code & 0xFF));

    result.insert(result.end(), frame.debug_data.begin(), frame.debug_data.end());
    return result;
}

byte_vector http2_framer::encode_window_update_frame(const http2_window_update_frame& frame) {
    byte_vector result = build_frame_header(http2_frame_type::WINDOW_UPDATE, 0, frame.stream_id, 4);
    uint32_t inc = frame.window_size_increment & 0x7FFFFFFF;
    result.push_back(static_cast<byte_t>((inc >> 24) & 0xFF));
    result.push_back(static_cast<byte_t>((inc >> 16) & 0xFF));
    result.push_back(static_cast<byte_t>((inc >> 8) & 0xFF));
    result.push_back(static_cast<byte_t>(inc & 0xFF));
    return result;
}

byte_vector http2_framer::encode_continuation_frame(const http2_continuation_frame& frame) {
    uint8_t flags = frame.end_headers ? HTTP2_FLAG_END_HEADERS : 0;
    byte_vector result =
            build_frame_header(http2_frame_type::CONTINUATION, flags, frame.stream_id, frame.header_block.size());
    result.insert(result.end(), frame.header_block.begin(), frame.header_block.end());
    return result;
}

void http2_framer::decode_frames(const byte_t* data, size_t len, frame_callback cb) {
    if (!partial_buffer_.empty()) {
        partial_buffer_.insert(partial_buffer_.end(), data, data + len);
        data = partial_buffer_.data();
        len = partial_buffer_.size();
    }

    byte_cursor cur(data, len);

    while (cur.remaining() >= 9) {
        size_t frame_start = cur.consumed_bytes();

        auto length_opt = cur.try_read_be24();
        auto type_opt = cur.try_read_byte();
        auto flags_opt = cur.try_read_byte();
        auto sid_opt = cur.try_read_be32();

        if (!length_opt || !type_opt || !flags_opt || !sid_opt) {
            partial_buffer_.assign(data + frame_start, data + len);
            return;
        }

        const uint32_t length = *length_opt;
        const uint8_t type = *type_opt;
        const uint8_t flags = *flags_opt;
        const uint32_t stream_id = *sid_opt & 0x7FFFFFFF;

        auto payload = cur.try_read_bytes(length);
        if (!payload) {
            partial_buffer_.assign(data + frame_start, data + len);
            return;
        }

        cb(static_cast<http2_frame_type>(type), flags, stream_id, payload->data(), length);
    }

    if (!cur.exhausted()) {
        size_t consumed = cur.consumed_bytes();
        partial_buffer_.assign(data + consumed, data + len);
    } else {
        partial_buffer_.clear();
    }
}

http2_stream::http2_stream(uint32_t stream_id) :
stream_id_(stream_id) {}

bool http2_stream::can_send_headers() const {
    return state_ == http2_stream_state::IDLE || state_ == http2_stream_state::RESERVED_LOCAL ||
           state_ == http2_stream_state::OPEN;
}

bool http2_stream::can_send_data() const { return state_ == http2_stream_state::OPEN; }

bool http2_stream::can_receive() const {
    return state_ == http2_stream_state::OPEN || state_ == http2_stream_state::HALF_CLOSED_REMOTE;
}

void http2_stream::on_send_headers(bool end_stream) {
    if (state_ == http2_stream_state::IDLE) {
        state_ = end_stream ? http2_stream_state::HALF_CLOSED_LOCAL : http2_stream_state::OPEN;
    } else if (state_ == http2_stream_state::RESERVED_LOCAL) {
        state_ = end_stream ? http2_stream_state::CLOSED : http2_stream_state::HALF_CLOSED_REMOTE;
    }
}

void http2_stream::on_send_data(bool end_stream) {
    if (end_stream) {
        if (state_ == http2_stream_state::OPEN) {
            state_ = http2_stream_state::HALF_CLOSED_LOCAL;
        } else if (state_ == http2_stream_state::HALF_CLOSED_REMOTE) {
            state_ = http2_stream_state::CLOSED;
        }
    }
}

void http2_stream::on_receive_headers(bool end_stream) {
    if (state_ == http2_stream_state::IDLE) {
        state_ = end_stream ? http2_stream_state::HALF_CLOSED_REMOTE : http2_stream_state::OPEN;
    } else if (state_ == http2_stream_state::RESERVED_REMOTE) {
        state_ = end_stream ? http2_stream_state::CLOSED : http2_stream_state::HALF_CLOSED_LOCAL;
    } else if (state_ == http2_stream_state::HALF_CLOSED_LOCAL && end_stream) {
        state_ = http2_stream_state::CLOSED;
    }
}

void http2_stream::on_receive_data(bool end_stream) {
    if (end_stream) {
        if (state_ == http2_stream_state::OPEN) {
            state_ = http2_stream_state::HALF_CLOSED_REMOTE;
        } else if (state_ == http2_stream_state::HALF_CLOSED_LOCAL) {
            state_ = http2_stream_state::CLOSED;
        }
    }
}

void http2_stream::on_send_rst_stream() { state_ = http2_stream_state::CLOSED; }

void http2_stream::on_receive_rst_stream() { state_ = http2_stream_state::CLOSED; }

void http2_stream::close() { state_ = http2_stream_state::CLOSED; }

void http2_stream::consume_local_window(uint32_t amount) {
    if (amount <= local_window_) {
        local_window_ -= amount;
    }
}

void http2_stream::consume_remote_window(uint32_t amount) {
    if (amount <= remote_window_) {
        remote_window_ -= amount;
    }
}

void http2_stream::add_local_window(uint32_t amount) { local_window_ += amount; }

void http2_stream::add_remote_window(uint32_t amount) { remote_window_ += amount; }

http2_flow_control::http2_flow_control(uint32_t initial_window) :
initial_window_(initial_window),
connection_window_(initial_window) {}

void http2_flow_control::set_initial_window(uint32_t size) { initial_window_ = size; }

bool http2_flow_control::can_send(uint32_t stream_id, uint32_t amount) const {
    if (amount > connection_window_) {
        return false;
    }
    auto it = stream_windows_.find(stream_id);
    const uint32_t stream_window = (it != stream_windows_.end()) ? it->second : initial_window_;
    return amount <= stream_window;
}

void http2_flow_control::consume(uint32_t stream_id, uint32_t amount) {
    connection_window_ -= amount;
    auto it = stream_windows_.find(stream_id);
    if (it == stream_windows_.end()) {
        stream_windows_[stream_id] = initial_window_ - amount;
    } else {
        it->second -= amount;
    }
}

void http2_flow_control::add_window(uint32_t stream_id, uint32_t amount) {
    if (stream_id == 0) {
        connection_window_ += amount;
    } else {
        auto it = stream_windows_.find(stream_id);
        if (it == stream_windows_.end()) {
            stream_windows_[stream_id] = initial_window_ + amount;
        } else {
            it->second += amount;
        }
    }
}

uint32_t http2_flow_control::window(uint32_t stream_id) const {
    auto it = stream_windows_.find(stream_id);
    return (it != stream_windows_.end()) ? it->second : initial_window_;
}

http2_settings::http2_settings() {
    set(http2_settings_id::HEADER_TABLE_SIZE, HTTP2_DEFAULT_HEADER_TABLE_SIZE);
    set(http2_settings_id::ENABLE_PUSH, 0);
    set(http2_settings_id::MAX_CONCURRENT_STREAMS, HTTP2_DEFAULT_MAX_CONCURRENT_STREAMS);
    set(http2_settings_id::INITIAL_WINDOW_SIZE, HTTP2_DEFAULT_INITIAL_WINDOW_SIZE);
    set(http2_settings_id::MAX_FRAME_SIZE, HTTP2_MAX_FRAME_SIZE);
    set(http2_settings_id::MAX_HEADER_LIST_SIZE, HTTP2_DEFAULT_MAX_HEADER_LIST_SIZE);
}

void http2_settings::set(http2_settings_id id, uint32_t value) { params_[static_cast<uint16_t>(id)] = value; }

uint32_t http2_settings::get(http2_settings_id id) const {
    auto it = params_.find(static_cast<uint16_t>(id));
    return (it != params_.end()) ? it->second : 0;
}

uint32_t http2_settings::header_table_size() const { return get(http2_settings_id::HEADER_TABLE_SIZE); }
bool http2_settings::enable_push() const { return get(http2_settings_id::ENABLE_PUSH) != 0; }
uint32_t http2_settings::max_concurrent_streams() const { return get(http2_settings_id::MAX_CONCURRENT_STREAMS); }
uint32_t http2_settings::initial_window_size() const { return get(http2_settings_id::INITIAL_WINDOW_SIZE); }
uint32_t http2_settings::max_frame_size() const { return get(http2_settings_id::MAX_FRAME_SIZE); }
uint32_t http2_settings::max_header_list_size() const { return get(http2_settings_id::MAX_HEADER_LIST_SIZE); }

void http2_settings::apply_remote_settings(const http2_settings_frame& frame) {
    for (const auto& entry: frame.entries) {
        uint32_t value = entry.value;
        switch (entry.id) {
            case http2_settings_id::MAX_FRAME_SIZE:
                if (value < 16384 || value > 16777215) {
                    continue;
                }
                break;
            case http2_settings_id::ENABLE_PUSH:
                if (value > 1) {
                    continue;
                }
                break;
            case http2_settings_id::INITIAL_WINDOW_SIZE:
                if (value > 0x7FFFFFFF) {
                    continue;
                }
                break;
            default:
                break;
        }
        params_[static_cast<uint16_t>(entry.id)] = value;
    }
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
