#include <NeForce/core/numeric/random.hpp>
#include <NeForce/network/websocket.hpp>
NEFORCE_BEGIN_NAMESPACE__

byte_vector websocket_session_base::build_frame(websocket_opcode opcode, const string& payload, bool masked) {
    byte_vector frame;
    frame.reserve(14 + payload.size());

    byte_t first_byte = 0x80;
    first_byte |= static_cast<byte_t>(opcode) & 0x0F;
    frame.push_back(first_byte);

    uint8_t second_byte = 0;
    if (masked) {
        second_byte |= 0x80;
    }

    size_t len = payload.size();
    if (len < 126) {
        second_byte |= static_cast<uint8_t>(len);
        frame.push_back(static_cast<byte_t>(second_byte));
    } else if (len <= 0xFFFF) {
        second_byte |= 126;
        frame.push_back(static_cast<byte_t>(second_byte));

        uint16_t net_len = endian::host_to_network(static_cast<uint16_t>(len));
        frame.insert(
            frame.end(),
            reinterpret_cast<byte_t*>(&net_len),
            reinterpret_cast<byte_t*>(&net_len) + 2);
    } else {
        second_byte |= 127;
        frame.push_back(static_cast<byte_t>(second_byte));

        auto net_len = endian::host_to_network<uint64_t>(len);
        frame.insert(
            frame.end(),
            reinterpret_cast<byte_t*>(&net_len),
            reinterpret_cast<byte_t*>(&net_len) + 8);
    }

    uint32_t masking_key = 0;
    if (masked) {
        thread_local random_mt tl_mt;
        masking_key = tl_mt.next_int<uint32_t>();
        frame.insert(
            frame.end(),
            reinterpret_cast<byte_t*>(&masking_key),
            reinterpret_cast<byte_t*>(&masking_key) + 4);
    }

    if (len > 0) {
        byte_vector payload_bytes(payload.begin(), payload.end());

        if (masked) {
            for (size_t i = 0; i < payload_bytes.size(); ++i) {
                payload_bytes[i] ^= reinterpret_cast<byte_t*>(&masking_key)[i % 4];
            }
        }

        frame.insert(frame.end(), payload_bytes.begin(), payload_bytes.end());
    }

    return frame;
}

NEFORCE_END_NAMESPACE__
