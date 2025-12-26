#pragma once
#include <MSTL/core/container/vector.hpp>
#include <MSTL/core/utility/packages.hpp>
#include <MSTL/network/tcp_server.hpp>

using namespace MSTL;

class message_protocol {
public:
    struct message_header : istringify<message_header> {
        uint32_t magic;      // 魔数
        uint32_t length;     // 体长度
        uint16_t checksum;   // 校验和

        string to_string() const {
            return "[HEADER] " + hexadecimal(magic).to_string() + " " +
                ::to_string(length) + " CHECK: " + ::to_string(checksum);
        }
    };

    static constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;
    static constexpr uint16_t PROTOCOL_VERSION = 1;
    static constexpr size_t HEADER_SIZE = sizeof(message_header);

private:
    using handle_sock_t = tcp_server::handle_sock_t;

    static uint16_t calculate_checksum(const char* data, size_t len) {
        uint32_t sum = 0;
        for (size_t i = 0; i < len; ++i) {
            sum += static_cast<uint8_t>(data[i]);
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        return static_cast<uint16_t>(sum);
    }

    static void header_to_network(message_header& header) {
        header.magic = htonl(header.magic);
        header.length = htonl(header.length);
        header.checksum = htons(header.checksum);
    }

    static void header_to_host(message_header& header) {
        header.magic = ntohl(header.magic);
        header.length = ntohl(header.length);
        header.checksum = ntohs(header.checksum);
    }

public:
    static vector<char> serialize(const string& message) {
        message_header header{};
        header.magic = MAGIC_NUMBER;
        header.length = static_cast<uint32_t>(message.size());
        header.checksum = calculate_checksum(message.data(), message.size());

        header_to_network(header);

        vector<char> packet(HEADER_SIZE + message.size());
        memory_copy(packet.data(), &header, HEADER_SIZE);
        memory_copy(packet.data() + HEADER_SIZE, message.data(), message.size());

        return packet;
    }

    static bool send_message(const handle_sock_t& client, const string& message) {
        vector<char> packet = serialize(message);

        size_t total_sent = 0;
        while (total_sent < packet.size()) {
            ssize_t sent = client.send(packet.data() + total_sent, packet.size() - total_sent);
            if (sent <= 0) {
                return false;
            }
            total_sent += sent;
        }
        return true;
    }

    static bool receive_message(const handle_sock_t& client, string& message, int timeout_ms = 5000) {
        message_header header{};
        char* header_ptr = reinterpret_cast<char*>(&header);
        size_t header_received = 0;

        auto start_time = steady_clock::now();
        while (header_received < HEADER_SIZE) {
            ssize_t n = client.receive(header_ptr + header_received, HEADER_SIZE - header_received);
            if (n <= 0) {
                if (duration_cast<milliseconds>(steady_clock::now() - start_time).count() > timeout_ms) {
                    return false;
                }
                this_thread::sleep_for(milliseconds(10));
                continue;
            }
            header_received += n;
        }

        header_to_host(header);

        if (header.magic != MAGIC_NUMBER) {
            return false;
        }

        if (header.length > 10 * 1024 * 1024) {
            return false;
        }

        vector<char> body(header.length);
        size_t body_received = 0;

        while (body_received < header.length) {
            ssize_t n = client.receive(body.data() + body_received, header.length - body_received);
            if (n <= 0) {
                if (duration_cast<milliseconds>(steady_clock::now() - start_time).count() > timeout_ms) {
                    return false;
                }
                this_thread::sleep_for(milliseconds(10));
                continue;
            }
            body_received += n;
        }

        const uint16_t calculated_checksum = calculate_checksum(body.data(), header.length);
        if (calculated_checksum != header.checksum) {
            return false;
        }

        message.assign(body.data(), header.length);
        return true;
    }
};