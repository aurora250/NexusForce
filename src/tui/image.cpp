#include <NeForce/core/algorithm/compare.hpp>
#include <NeForce/tui/image.hpp>
#ifdef NEFORCE_SUPPORT_ZLIB
#    include <climits>
#    include <zlib.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    constexpr byte_t png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    constexpr size_t kitty_payload_limit = 4096;
    constexpr size_t max_png_decoded_size = 512ULL * 1024ULL * 1024ULL;

    struct sixel_color {
        byte_t red_;
        byte_t green_;
        byte_t blue_;
    };

    uint32_t read_be32(const byte_t* data) noexcept {
        return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
    }

    bool chunk_type_is(const byte_t* type, const char* name) noexcept {
        for (int i = 0; i < 4; ++i) {
            if (type[i] != static_cast<byte_t>(name[i])) {
                return false;
            }
        }
        return true;
    }

    void append_number(string& out, uint64_t value) {
        char buffer[20];
        size_t position = sizeof(buffer);
        do {
            buffer[--position] = static_cast<char>('0' + static_cast<int>(value % 10));
            value /= 10;
        } while (value != 0);
        out.append(buffer + position, sizeof(buffer) - position);
    }

    void append_escape_backslash(string& out) {
        out.push_back('\x1b');
        out.push_back('\\');
    }

    string encode_base64(const byte_t* data, const size_t size) {
        constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        string result;
        if (size == 0) {
            return result;
        }
        result.reserve((size + 2) / 3 * 4);
        size_t index = 0;
        while (index + 2 < size) {
            const uint32_t value = (static_cast<uint32_t>(data[index]) << 16) |
                                   (static_cast<uint32_t>(data[index + 1]) << 8) |
                                   static_cast<uint32_t>(data[index + 2]);
            result.push_back(alphabet[(value >> 18) & 0x3F]);
            result.push_back(alphabet[(value >> 12) & 0x3F]);
            result.push_back(alphabet[(value >> 6) & 0x3F]);
            result.push_back(alphabet[value & 0x3F]);
            index += 3;
        }
        if (index < size) {
            uint32_t value = static_cast<uint32_t>(data[index]) << 16;
            if (index + 1 < size) {
                value |= static_cast<uint32_t>(data[index + 1]) << 8;
            }
            result.push_back(alphabet[(value >> 18) & 0x3F]);
            result.push_back(alphabet[(value >> 12) & 0x3F]);
            result.push_back(index + 1 < size ? alphabet[(value >> 6) & 0x3F] : '=');
            result.push_back('=');
        }
        return result;
    }

    int paeth_predictor(const int left, const int up, const int up_left) noexcept {
        const int estimate = left + up - up_left;
        const int distance_left = estimate > left ? estimate - left : left - estimate;
        const int distance_up = estimate > up ? estimate - up : up - estimate;
        const int distance_up_left = estimate > up_left ? estimate - up_left : up_left - estimate;
        if (distance_left <= distance_up && distance_left <= distance_up_left) {
            return left;
        }
        return distance_up <= distance_up_left ? up : up_left;
    }

    int palette_distance(const sixel_color& color, const int red, const int green, const int blue) noexcept {
        const int delta_red = red - static_cast<int>(color.red_);
        const int delta_green = green - static_cast<int>(color.green_);
        const int delta_blue = blue - static_cast<int>(color.blue_);
        return delta_red * delta_red + delta_green * delta_green + delta_blue * delta_blue;
    }

    vector<sixel_color> make_sixel_palette() {
        vector<sixel_color> palette;
        palette.reserve(256);
        constexpr byte_t levels[6] = {0, 51, 102, 153, 204, 255};
        for (int red = 0; red < 6; ++red) {
            for (int green = 0; green < 6; ++green) {
                for (int blue = 0; blue < 6; ++blue) {
                    palette.push_back(sixel_color{levels[red], levels[green], levels[blue]});
                }
            }
        }
        for (int index = 0; index < 40; ++index) {
            const auto value = static_cast<byte_t>((index * 255 + 19) / 39);
            palette.push_back(sixel_color{value, value, value});
        }
        return palette;
    }

    const vector<sixel_color>& sixel_palette() {
        static const vector<sixel_color> palette = make_sixel_palette();
        return palette;
    }

    int nearest_sixel_color(const vector<sixel_color>& palette, const int red, const int green,
                            const int blue) noexcept {
        int best = 0;
        int best_distance = 0x7FFFFFFF;
        for (size_t index = 0; index < palette.size(); ++index) {
            const int distance = palette_distance(palette[index], red, green, blue);
            if (distance < best_distance) {
                best_distance = distance;
                best = static_cast<int>(index);
                if (distance == 0) {
                    break;
                }
            }
        }
        return best;
    }

    void append_sixel_run(string& out, const int value, const int count) {
        if (count <= 0) {
            return;
        }
        if (count > 1) {
            out.push_back('!');
            append_number(out, static_cast<uint64_t>(count));
        }
        out.push_back(static_cast<char>('?' + value));
    }
} // namespace


optional<image_source> decode_png(const string_view bytes) {
#ifdef NEFORCE_SUPPORT_ZLIB
    if (bytes.size() < sizeof(png_signature)) {
        return none;
    }
    const auto* data = reinterpret_cast<const byte_t*>(bytes.data());
    for (size_t index = 0; index < sizeof(png_signature); ++index) {
        if (data[index] != png_signature[index]) {
            return none;
        }
    }

    uint32_t width = 0;
    uint32_t height = 0;
    size_t bytes_per_pixel = 0;
    bool have_header = false;
    bool have_end = false;
    vector<byte_t> compressed;
    size_t position = sizeof(png_signature);

    while (position < bytes.size()) {
        if (bytes.size() - position < 12) {
            return none;
        }
        const uint32_t length = read_be32(data + position);
        const size_t data_start = position + 8;
        if (static_cast<size_t>(length) > bytes.size() - data_start - 4) {
            return none;
        }
        const byte_t* type = data + position + 4;
        const byte_t* chunk_data = data + data_start;

        if (!have_header) {
            if (!chunk_type_is(type, "IHDR") || length != 13) {
                return none;
            }
            width = read_be32(chunk_data);
            height = read_be32(chunk_data + 4);
            const byte_t bit_depth = chunk_data[8];
            const byte_t color_type = chunk_data[9];
            const byte_t compression_method = chunk_data[10];
            const byte_t filter_method = chunk_data[11];
            const byte_t interlace_method = chunk_data[12];
            if (width == 0 || height == 0 || width > 0x7FFFFFFFU || height > 0x7FFFFFFFU) {
                return none;
            }
            if (bit_depth != 8 || (color_type != 2 && color_type != 6)) {
                return none;
            }
            if (compression_method != 0 || filter_method != 0 || interlace_method != 0) {
                return none;
            }
            bytes_per_pixel = color_type == 6 ? 4 : 3;
            have_header = true;
        } else if (chunk_type_is(type, "IHDR")) {
            return none;
        } else if (chunk_type_is(type, "IDAT")) {
            compressed.insert(compressed.end(), chunk_data, chunk_data + length);
        } else if (chunk_type_is(type, "IEND")) {
            if (length != 0) {
                return none;
            }
            have_end = true;
            break;
        }
        position = data_start + static_cast<size_t>(length) + 4;
    }

    if (!have_header || !have_end || compressed.empty()) {
        return none;
    }

    constexpr auto size_limit = static_cast<size_t>(-1);
    const auto pixel_width = static_cast<size_t>(width);
    const auto pixel_height = static_cast<size_t>(height);
    if (pixel_width > size_limit / bytes_per_pixel) {
        return none;
    }
    const size_t stride = pixel_width * bytes_per_pixel;
    if (stride >= max_png_decoded_size) {
        return none;
    }
    const size_t row_size = stride + 1;
    if (pixel_height > max_png_decoded_size / row_size) {
        return none;
    }
    const size_t raw_size = pixel_height * row_size;
    if (pixel_height > size_limit / pixel_width) {
        return none;
    }
    const size_t pixel_count = pixel_width * pixel_height;
    if (pixel_count > max_png_decoded_size / 4) {
        return none;
    }
    if (compressed.size() > numeric_traits<size_t>::max()) {
        return none;
    }

    vector<byte_t> raw(raw_size);
    auto decoded_size = static_cast<::uLongf>(raw_size);
    const int status =
            ::uncompress(raw.data(), &decoded_size, compressed.data(), static_cast<::uLong>(compressed.size()));
    if (status != Z_OK || decoded_size != static_cast<::uLongf>(raw_size)) {
        return none;
    }

    vector<byte_t> previous(stride, 0);
    vector<byte_t> current(stride, 0);
    vector<byte_t> rgba(pixel_count * 4);
    for (size_t y = 0; y < pixel_height; ++y) {
        const size_t row_offset = y * row_size;
        const int filter = static_cast<int>(raw[row_offset]);
        if (filter < 0 || filter > 4) {
            return none;
        }
        const byte_t* source = raw.data() + row_offset + 1;
        for (size_t index = 0; index < stride; ++index) {
            const int left = index >= bytes_per_pixel ? static_cast<int>(current[index - bytes_per_pixel]) : 0;
            const int up = static_cast<int>(previous[index]);
            const int up_left = index >= bytes_per_pixel ? static_cast<int>(previous[index - bytes_per_pixel]) : 0;
            int predictor = 0;
            switch (filter) {
                case 0:
                    predictor = 0;
                    break;
                case 1:
                    predictor = left;
                    break;
                case 2:
                    predictor = up;
                    break;
                case 3:
                    predictor = (left + up) / 2;
                    break;
                case 4:
                    predictor = paeth_predictor(left, up, up_left);
                    break;
                default:
                    return none;
            }
            current[index] = static_cast<byte_t>(static_cast<int>(source[index]) + predictor);
        }

        byte_t* destination = rgba.data() + (y * pixel_width) * 4;
        for (size_t x = 0; x < pixel_width; ++x) {
            const size_t source_offset = x * bytes_per_pixel;
            const size_t destination_offset = x * 4;
            destination[destination_offset] = current[source_offset];
            destination[destination_offset + 1] = current[source_offset + 1];
            destination[destination_offset + 2] = current[source_offset + 2];
            destination[destination_offset + 3] = bytes_per_pixel == 4 ? current[source_offset + 3] : 0xFF;
        }
        previous.swap(current);
    }

    image_source result;
    result.width = static_cast<int>(width);
    result.height = static_cast<int>(height);
    result.rgba = move(rgba);
    return optional<image_source>(move(result));
#else
    (void) bytes;
    return none;
#endif
}

image_protocol detect_image_protocol(const string_view term, const string_view term_program,
                                     const string_view kitty_window_id, const string_view sixel_support) {
    if (term == "xterm-kitty" || !kitty_window_id.empty()) {
        return image_protocol::kitty;
    }
    if (term_program.contains("iTerm.app") || term_program.contains("WezTerm")) {
        return image_protocol::iterm2;
    }
    if (!sixel_support.empty() || term.contains("sixel")) {
        return image_protocol::sixel;
    }
    return image_protocol::none;
}

string encode_kitty(const image_source& image, const image_cell_box& box, const uint32_t image_id,
                    const bool transmit) {
    string result;
    if (!image.valid()) {
        return result;
    }
    if (!transmit) {
        result += "\x1b_Ga=p,i=";
        append_number(result, static_cast<uint64_t>(image_id));
        result += ",p=1";
        if (box.columns > 0 && box.rows > 0) {
            result += ",c=";
            append_number(result, static_cast<uint64_t>(box.columns));
            result += ",r=";
            append_number(result, static_cast<uint64_t>(box.rows));
        }
        append_escape_backslash(result);
        return result;
    }
    const string encoded = encode_base64(image.rgba.data(), image.rgba.size());
    if (encoded.empty()) {
        return result;
    }

    result.reserve(encoded.size() + encoded.size() / kitty_payload_limit * 64 + 128);
    size_t offset = 0;
    bool first = true;
    while (offset < encoded.size()) {
        const size_t remaining = encoded.size() - offset;
        const size_t length = min(remaining, kitty_payload_limit);
        const bool last = offset + length == encoded.size();
        result += "\x1b_G";
        if (first) {
            result += "f=32,s=";
            append_number(result, static_cast<uint64_t>(image.width));
            result += ",v=";
            append_number(result, static_cast<uint64_t>(image.height));
            result += ",a=";
            result.push_back('T');
            result += ",i=";
            append_number(result, static_cast<uint64_t>(image_id));
            result += ",p=1";
        }
        result += ",m=";
        result.push_back(last ? '0' : '1');
        if (first) {
            result += ",q=2";
        }
        result.push_back(';');
        result.append(encoded.data() + offset, length);
        append_escape_backslash(result);
        first = false;
        offset += length;
    }
    return result;
}

string encode_iterm2(const image_source& image, const image_cell_box& box) {
    string result;
    if (!image.valid()) {
        return result;
    }
    const string encoded = encode_base64(image.rgba.data(), image.rgba.size());
    const int columns = box.columns > 0 ? box.columns : 0;
    const int rows = box.rows > 0 ? box.rows : 0;

    result.reserve(encoded.size() + 96);
    result += "\x1b]1337;File=inline=1;width=";
    append_number(result, static_cast<uint64_t>(columns));
    result += ";height=";
    append_number(result, static_cast<uint64_t>(rows));
    result += ";preserveAspectRatio=1:";
    result += encoded;
    result.push_back('\a');
    return result;
}

string encode_sixel(const image_source& image, const image_cell_box& box) {
    string result;
    (void) box;
    if (!image.valid()) {
        return result;
    }

    const auto pixel_width = static_cast<size_t>(image.width);
    const auto pixel_height = static_cast<size_t>(image.height);
    const vector<sixel_color>& palette = sixel_palette();
    vector<int16_t> indices(pixel_width * pixel_height, -1);
    for (size_t y = 0; y < pixel_height; ++y) {
        const byte_t* row = image.rgba.data() + y * pixel_width * 4;
        for (size_t x = 0; x < pixel_width; ++x) {
            const byte_t* pixel = row + x * 4;
            if (pixel[3] == 0) {
                continue;
            }
            indices[y * pixel_width + x] = static_cast<int16_t>(nearest_sixel_color(
                    palette, static_cast<int>(pixel[0]), static_cast<int>(pixel[1]), static_cast<int>(pixel[2])));
        }
    }

    result.reserve(pixel_width * pixel_height / 2 + 64);
    result += "\x1bPq";
    result.push_back('"');
    append_number(result, 1);
    result.push_back(';');
    append_number(result, 1);
    result.push_back(';');
    append_number(result, static_cast<uint64_t>(image.width));
    result.push_back(';');
    append_number(result, static_cast<uint64_t>(image.height));

    const int bands = (image.height - 1) / 6 + 1;
    for (int band = 0; band < bands; ++band) {
        const int first_row = band * 6;
        bool used[256] = {};
        for (int row = first_row; row < first_row + 6 && row < image.height; ++row) {
            for (size_t x = 0; x < pixel_width; ++x) {
                const int index = static_cast<int>(indices[static_cast<size_t>(row) * pixel_width + x]);
                if (index >= 0) {
                    used[index] = true;
                }
            }
        }

        for (int color = 0; color < 256; ++color) {
            if (!used[color]) {
                continue;
            }
            result.push_back('#');
            append_number(result, static_cast<uint64_t>(color));
            int run_value = -1;
            int run_count = 0;
            for (size_t x = 0; x < pixel_width; ++x) {
                int mask = 0;
                for (int bit = 0; bit < 6; ++bit) {
                    const int row = first_row + bit;
                    if (row >= image.height) {
                        break;
                    }
                    if (static_cast<int>(indices[static_cast<size_t>(row) * pixel_width + x]) == color) {
                        mask |= 1 << bit;
                    }
                }
                if (mask == run_value) {
                    ++run_count;
                } else {
                    append_sixel_run(result, run_value, run_count);
                    run_value = mask;
                    run_count = 1;
                }
            }
            append_sixel_run(result, run_value, run_count);
            result.push_back('$');
        }
        if (band + 1 < bands) {
            result.push_back('-');
        }
    }
    append_escape_backslash(result);
    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
