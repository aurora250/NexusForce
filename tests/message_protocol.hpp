#ifndef SERVER_MESSAGE_PROTOCOL_HPP__
#define SERVER_MESSAGE_PROTOCOL_HPP__
#include <MSTL/core/container/vector.hpp>
#include <MSTL/core/utility/packages.hpp>
#include <MSTL/network/tcp_server.hpp>
#include <MSTL/core/utility/optional.hpp>
#include <MSTL/core/utility/hexadecimal.hpp>
#include <MSTL/core/time/datetime.hpp>

using namespace MSTL;

#ifdef MSTL_PLATFORM_WINDOWS__
#define htobe16(x) htons(x)
#define htobe32(x) htonl(x)
#define htobe64(x) _byteswap_uint64(x)

#define be16toh(x) ntohs(x)
#define be32toh(x) ntohl(x)
#define be64toh(x) _byteswap_uint64(x)
#endif


enum class MESSAGE_TYPE : uint8_t {
    SUBMIT_TASK_REQUEST = 0X01, SUBMIT_TASK_RESPONSE = 0X02,
    ERROR_RESPONSE = 0X03,
    TASK_STATUS_REQUEST = 0X04, TASK_STATUS_RESPONSE = 0X05,
    CANCEL_TASK_REQUEST = 0X06, CANCEL_TASK_RESPONSE = 0X07,
    HEARTBEAT_REQUEST = 0X08, HEARTBEAT_RESPONSE = 0X09,
    STATISTICS_REQUEST = 0X11, STATISTICS_RESPONSE = 0X12,
    REGISTER_WORKER_REQUEST = 0X14, REGISTER_WORKER_RESPONSE = 0X15,
    TASK_COMPLETE_REQUEST = 0X20, TASK_COMPLETE_RESPONSE = 0X21,
};


struct message_header : istringify<message_header> {
    static constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;

    uint32_t magic;
    uint32_t length;
    uint64_t timestamp;
    uint16_t checksum;
    MESSAGE_TYPE type;

    string to_string() const {
        return
            "[HEADER] " + ::to_string(hexadecimal(magic)) + " " +
            ::to_string(length) + " TYPE: " + ::to_string(hexadecimal(static_cast<uint64_t>(type))) +
            " TIME: " + ::to_string(timestamp) + " CHECK: " + ::to_string(checksum);
    }

    void to_network() {
        magic = htonl(magic);
        length = htonl(length);
        timestamp = htobe64(timestamp);
        checksum = htons(checksum);
    }

    void to_host() {
        magic = ntohl(magic);
        length = ntohl(length);
        timestamp = be64toh(timestamp);
        checksum = ntohs(checksum);
    }

    static uint16_t calculate_checksum(const char* data,
        const size_t len, MESSAGE_TYPE type, uint64_t timestamp) {
        uint32_t sum = 0;

        uint8_t type_byte = static_cast<uint8_t>(type);
        sum += type_byte;
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


struct message : istringify<message> {
    message_header header;
    vector<char> body;

    string to_string() const {
        return ::to_string(header) + "\n[Body]" + string{body.data(), body.size()};
    }

    void set_body(const string& data, MESSAGE_TYPE type) {
        body.resize(data.size());
        memory_copy(body.data(), data.data(), data.size());
        header.length = static_cast<uint32_t>(data.size());
        header.type = type;
        header.timestamp = timestamp::now();
        header.checksum = message_header::calculate_checksum(
            data.data(), data.size(), type, header.timestamp);
    }

    vector<char> serialize() {
        header.to_network();

        constexpr size_t HEADER_SIZE = sizeof(message_header);
        vector<char> packet(HEADER_SIZE + body.size());
        memory_copy(packet.data(), &header, HEADER_SIZE);
        memory_copy(packet.data() + HEADER_SIZE, body.data(), body.size());

        return packet;
    }

    static vector<char> serialize(const string& message, MESSAGE_TYPE type) {
        message_header header{};
        memory_zero(&header, sizeof(message_header));
        header.magic = message_header::MAGIC_NUMBER;
        header.length = static_cast<uint32_t>(message.size());
        header.type = type;
        header.timestamp = timestamp::now();
        header.checksum = message_header::calculate_checksum(
            message.data(), message.size(), type, header.timestamp);
        header.to_network();

        constexpr size_t HEADER_SIZE = sizeof(message_header);
        vector<char> packet(HEADER_SIZE + message.size());
        memory_copy(packet.data(), &header, HEADER_SIZE);
        memory_copy(packet.data() + HEADER_SIZE, message.data(), message.size());

        return packet;
    }

    static bool deserialize(const char* data, const size_t data_len, message& msg) {
        constexpr size_t HEADER_SIZE = sizeof(message_header);
        if (data_len < HEADER_SIZE) {
            return false;
        }

        memory_copy(&msg.header, data, HEADER_SIZE);
        msg.header.to_host();

        if (msg.header.magic != message_header::MAGIC_NUMBER) {
            return false;
        }
        if (msg.header.length > data_len - HEADER_SIZE) {
            return false;
        }
        if (msg.header.length > 10 * 1024 * 1024) {
            return false;
        }

        msg.body.resize(msg.header.length);
        memory_copy(msg.body.data(), data + HEADER_SIZE, msg.header.length);

        const uint16_t calculated_checksum = message_header::calculate_checksum(
            msg.body.data(), msg.header.length, msg.header.type, msg.header.timestamp);
        if (calculated_checksum != msg.header.checksum) {
            return false;
        }
        return true;
    }
};


class message_transport {
private:
    using handle_sock_t = tcp_server::handle_sock_t;

    static bool send_raw(const handle_sock_t& client, const char* data, const size_t length) {
        size_t total_sent = 0;
        while (total_sent < length) {
            const ssize_t sent = client.send(data + total_sent, length - total_sent);
            if (sent <= 0) {
                return false;
            }
            total_sent += sent;
        }
        return true;
    }

    static bool receive_fixed(const handle_sock_t& client,
        char* buffer, size_t length, milliseconds timeout_ms) {
        auto start_time = steady_clock::now();
        size_t total_received = 0;

        while (total_received < length) {
            ssize_t n = client.receive(buffer + total_received, length - total_received);
            if (n > 0) {
                total_received += n;
            } else if (n == 0) {
                return false;
            } else {
                if (duration_cast<milliseconds>(steady_clock::now() - start_time) > timeout_ms) {
                    return false;
                }
                this_thread::sleep_for(milliseconds(10));
            }
        }
        return true;
    }

public:
    static constexpr size_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024;

    static bool send_message(const handle_sock_t& client, message& msg) {
        vector<char> packet = msg.serialize();
        return send_raw(client, packet.data(), packet.size());
    }

    static bool send_message(const handle_sock_t& client, const vector<char>& packet) {
        return send_raw(client, packet.data(), packet.size());
    }

    static bool send_message(const handle_sock_t& client, const string& data, MESSAGE_TYPE type) {
        message msg{};
        memory_zero(&msg.header, sizeof(message_header));
        msg.header.magic = message_header::MAGIC_NUMBER;
        msg.set_body(data, type);
        return send_message(client, msg);
    }

    static bool receive_message(const handle_sock_t& client,
        message& msg, milliseconds timeout_ms = milliseconds(5000)) {
        constexpr size_t HEADER_SIZE = sizeof(message_header);
        vector<char> header_data(HEADER_SIZE);

        if (!receive_fixed(client, header_data.data(), HEADER_SIZE, timeout_ms)) {
            return false;
        }
        message_header temp_header{};
        memory_copy(&temp_header, header_data.data(), HEADER_SIZE);
        temp_header.to_host();

        if (temp_header.magic != message_header::MAGIC_NUMBER) {
            return false;
        }
        if (temp_header.length > MAX_MESSAGE_SIZE) {
            return false;
        }

        vector<char> full_data(HEADER_SIZE + temp_header.length);
        memory_copy(full_data.data(), header_data.data(), HEADER_SIZE);

        if (temp_header.length > 0) {
            if (!receive_fixed(client, full_data.data() + HEADER_SIZE, temp_header.length, timeout_ms)) {
                return false;
            }
        }
        if (!message::deserialize(full_data.data(), full_data.size(), msg)) {
            return false;
        }
        return true;
    }

    static bool receive_message_body(const handle_sock_t& client,
        string& body, milliseconds timeout_ms = milliseconds(5000)) {
        message msg;
        if (!receive_message(client, msg, timeout_ms)) {
            return false;
        }
        body = string{msg.body.data(), msg.header.length};
        return true;
    }
};

#endif // SERVER_MESSAGE_PROTOCOL_HPP__
