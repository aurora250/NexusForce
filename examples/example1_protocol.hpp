#ifndef EXAMPLE1_PROTOCOL_HPP__
#define EXAMPLE1_PROTOCOL_HPP__
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/tcp/tcp_server.hpp>
#include <NeForce/core/utility/optional.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/time/datetime.hpp>
#include <NeForce/core/string/string_util.hpp>
#include <NeForce/core/utility/byte_size.hpp>

enum class message_type : uint8_t {
    SUBMIT_TASK_REQUEST = 0X01,
    SUBMIT_TASK_RESPONSE = 0X02,
    ERROR_RESPONSE = 0X03,
    TASK_STATUS_REQUEST = 0X04,
    TASK_STATUS_RESPONSE = 0X05,
    CANCEL_TASK_REQUEST = 0X06,
    CANCEL_TASK_RESPONSE = 0X07,
    HEARTBEAT_REQUEST = 0X08,
    HEARTBEAT_RESPONSE = 0X09,
    STATISTICS_REQUEST = 0X11,
    STATISTICS_RESPONSE = 0X12,
    REGISTER_WORKER_REQUEST = 0X14,
    REGISTER_WORKER_RESPONSE = 0X15,
    TASK_COMPLETE_REQUEST = 0X20,
    TASK_COMPLETE_RESPONSE = 0X21,
    UNKNOWN = 0xFF,
};

struct task_metadata {
    uint64_t task_id;
    uint32_t priority;
    uint64_t submit_time;

    neforce::string serialize() const {
        return neforce::to_string(task_id) + ":" + neforce::to_string(priority) + ":" + neforce::to_string(submit_time);
    }

    static neforce::optional<task_metadata> deserialize(const neforce::string& data) {
        auto parts = split(data.view(), ":");
        if (parts.size() != 3) {
            neforce::printcln(neforce::color::red(), "Failed to deserialize metadata, parts:", parts.size());
            return {};
        }

        task_metadata meta{};
        try {
            meta.task_id = to_uint64(parts[0]);
            meta.priority = to_uint32(parts[1]);
            meta.submit_time = to_uint64(parts[2]);
            return meta;
        } catch (...) {
            neforce::printcln(neforce::color::red(), "Failed to convert metadata values");
            return {};
        }
    }
};


#pragma pack(push, 1)
struct message_header : neforce::istringify<message_header> {
    static constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;

    uint32_t magic = 0;
    uint32_t length = 0;
    uint64_t timestamp = 0;
    uint16_t checksum = 0;
    message_type type = message_type::UNKNOWN;

    neforce::string to_string() const {
        return "[HEADER] " + neforce::to_string(neforce::hexadecimal(magic)) + " " + neforce::to_string(length) +
               " TYPE: " + neforce::to_string(neforce::hexadecimal(static_cast<uint64_t>(type))) +
               " TIME: " + neforce::to_string(timestamp) + " CHECK: " + neforce::to_string(checksum);
    }

    message_header to_network() const noexcept {
        message_header copy = *this;
        copy.magic = neforce::endian::host_to_network(magic);
        copy.length = neforce::endian::host_to_network(length);
        copy.timestamp = neforce::endian::host_to_be(timestamp);
        copy.checksum = neforce::endian::host_to_network(checksum);
        return copy;
    }

    void to_host() noexcept {
        magic = neforce::endian::network_to_host(magic);
        length = neforce::endian::network_to_host(length);
        timestamp = neforce::endian::be_to_host(timestamp);
        checksum = neforce::endian::network_to_host(checksum);
    }

    static uint16_t calculate_checksum(const char* data, const size_t len, message_type type,
                                       uint64_t timestamp) noexcept {
        uint32_t sum = 0;

        sum += static_cast<uint8_t>(type);
        sum = (sum & 0xFFFF) + (sum >> 16);

        for (int i = 0; i < 8; ++i) {
            sum += static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF);
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        for (size_t i = 0; i < len; ++i) {
            sum += static_cast<uint8_t>(data[i]);
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        return static_cast<uint16_t>(sum);
    }
};
#pragma pack(pop)


struct message : neforce::istringify<message> {
    message_header header{};
    neforce::vector<char> body{};
    neforce::optional<task_metadata> metadata;

    neforce::string to_string() const {
        return neforce::to_string(header) + "\n[Body]" + neforce::string{body.data(), body.size()};
    }

    void set_body(const neforce::string& data, message_type type) {
        body.resize(data.size());
        neforce::memory_copy(body.data(), data.data(), data.size());
        header.length = static_cast<uint32_t>(data.size());
        header.type = type;
        header.timestamp = neforce::timestamp::now().value();
        header.checksum = message_header::calculate_checksum(data.data(), data.size(), type, header.timestamp);
    }

    neforce::vector<char> serialize() const {
        message_header net_header = header.to_network();

        constexpr size_t HDR = sizeof(message_header);
        neforce::vector<char> packet(HDR + body.size());
        neforce::memory_copy(packet.data(), &net_header, HDR);
        neforce::memory_copy(packet.data() + HDR, body.data(), body.size());
        return packet;
    }

    static neforce::vector<char> serialize(const neforce::string& body, message_type type) {
        message_header header{};
        neforce::memory_zero(&header, sizeof(message_header));
        header.magic = message_header::MAGIC_NUMBER;
        header.length = static_cast<uint32_t>(body.size());
        header.type = type;
        header.timestamp = neforce::timestamp::now().value();
        header.checksum = message_header::calculate_checksum(body.data(), body.size(), type, header.timestamp);

        message_header net_header = header.to_network();

        constexpr size_t HDR = sizeof(message_header);
        neforce::vector<char> packet(HDR + body.size());
        neforce::memory_copy(packet.data(), &net_header, HDR);
        neforce::memory_copy(packet.data() + HDR, body.data(), body.size());

        return packet;
    }

    static bool deserialize(const char* data, const size_t data_len, message& msg) {
        using namespace neforce::literals;

        constexpr size_t HDR = sizeof(message_header);
        static const auto MBL = 10_MB;

        if (data_len < HDR) {
            return false;
        }

        neforce::memory_copy(&msg.header, data, HDR);
        msg.header.to_host();

        if (msg.header.magic != message_header::MAGIC_NUMBER) {
            return false;
        }
        if (msg.header.length > data_len - HDR) {
            return false;
        }
        if (msg.header.length > MBL.bytes()) {
            return false;
        }

        msg.body.resize(msg.header.length);
        neforce::memory_copy(msg.body.data(), data + HDR, msg.header.length);

        const uint16_t calc = message_header::calculate_checksum(msg.body.data(), msg.header.length, msg.header.type,
                                                                 msg.header.timestamp);
        if (calc != msg.header.checksum) {
            return false;
        }
        return true;
    }

    void set_body_with_metadata(const neforce::string& data, message_type type, const task_metadata& meta) {
        const neforce::string full = meta.serialize() + "|" + data;
        set_body(full, type);
        metadata = meta;
    }

    static bool extract_metadata(const neforce::string& full_data, task_metadata& meta, neforce::string& actual_data) {
        const size_t pos = full_data.find('|');
        if (pos == neforce::string::npos) {
            return false;
        }

        auto meta_opt = task_metadata::deserialize(full_data.head(pos));
        if (!meta_opt) {
            return false;
        }

        meta = *meta_opt;
        actual_data = full_data.tail(pos + 1);
        return true;
    }
};


class message_transport {
private:
    static bool send_raw(neforce::tcp_socket& sock, const char* data, size_t length) {
        size_t sent_total = 0;
        while (sent_total < length) {
            const neforce::ssize_t n = sock.send({data + sent_total, length - sent_total});
            if (n <= 0) {
                return false;
            }
            sent_total += static_cast<size_t>(n);
        }
        return true;
    }

    static bool receive_fixed(neforce::tcp_socket& sock, char* buf, size_t length, neforce::milliseconds timeout_ms) {
        const auto start = neforce::steady_clock::now();
        size_t total = 0;
        while (total < length) {
            neforce::ssize_t n = 0;
            try {
                n = sock.receive({buf + total, length - total});
            } catch (...) {
                auto elapsed = neforce::time_cast<neforce::milliseconds>(neforce::steady_clock::now() - start);
                if (elapsed > timeout_ms) {
                    return false;
                }
                neforce::this_thread::sleep_for(neforce::milliseconds(5));
            }
            if (n > 0) {
                total += static_cast<size_t>(n);
            } else if (n == 0) {
                return false;
            } else {
                auto elapsed = neforce::time_cast<neforce::milliseconds>(neforce::steady_clock::now() - start);
                if (elapsed > timeout_ms) {
                    return false;
                }
                neforce::this_thread::sleep_for(neforce::milliseconds(5));
            }
        }
        return true;
    }

public:
    static bool send_message(neforce::tcp_socket& sock, const message& msg) {
        auto packet = msg.serialize();
        return send_raw(sock, packet.data(), packet.size());
    }

    static bool send_message(neforce::tcp_socket& sock, const neforce::vector<char>& packet) {
        return send_raw(sock, packet.data(), packet.size());
    }

    static bool send_message(neforce::tcp_socket& sock, const neforce::string& data, message_type type) {
        auto packet = message::serialize(data, type);
        return send_raw(sock, packet.data(), packet.size());
    }

    static bool receive_message(neforce::tcp_socket& sock, message& msg,
                                neforce::milliseconds timeout_ms = neforce::milliseconds(5000)) {
        using namespace neforce::literals;
        static const auto MAX_BODY = 10_MB;

        constexpr size_t HDR = sizeof(message_header);
        neforce::vector<char> hdr_buf(HDR);

        if (!receive_fixed(sock, hdr_buf.data(), HDR, timeout_ms)) {
            return false;
        }

        message_header tmp{};
        neforce::memory_copy(&tmp, hdr_buf.data(), HDR);
        tmp.to_host();

        if (tmp.magic != message_header::MAGIC_NUMBER) {
            return false;
        }
        if (tmp.length > MAX_BODY.bytes()) {
            return false;
        }

        neforce::vector<char> full(HDR + tmp.length);
        neforce::memory_copy(full.data(), hdr_buf.data(), HDR);

        if (tmp.length > 0) {
            if (!receive_fixed(sock, full.data() + HDR, tmp.length, timeout_ms)) {
                return false;
            }
        }

        return message::deserialize(full.data(), full.size(), msg);
    }

    static bool receive_message_body(neforce::tcp_socket& client, neforce::string& body,
                                     neforce::milliseconds timeout_ms = neforce::milliseconds(5000)) {
        message msg;
        if (!receive_message(client, msg, timeout_ms)) {
            return false;
        }
        body = neforce::string{msg.body.data(), msg.header.length};
        return true;
    }
};

#endif // EXAMPLE1_PROTOCOL_HPP__
