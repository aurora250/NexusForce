#include <NeForce/core/numeric/random.hpp>
#include <NeForce/network/websocket.hpp>
NEFORCE_BEGIN_NAMESPACE__

byte_vector websocket_session_base::build_frame(websocket_opcode opcode, const string& payload, bool masked) {
    byte_vector frame;
    frame.reserve(14 + payload.size());
    frame.push_back(static_cast<byte_t>(0x80 | (static_cast<uint8_t>(opcode) & 0x0F)));

    const size_t len = payload.size();
    const byte_t second = masked ? 0x80 : 0x00;

    if (len < 126) {
        frame.push_back(static_cast<byte_t>(second | static_cast<byte_t>(len)));
    } else if (len <= 0xFFFF) {
        frame.push_back(static_cast<byte_t>(second | 126));
        const uint16_t net_len = endian::host_to_network(static_cast<uint16_t>(len));
        frame.insert(frame.end(), reinterpret_cast<const byte_t*>(&net_len),
                     reinterpret_cast<const byte_t*>(&net_len) + 2);
    } else {
        frame.push_back(static_cast<byte_t>(second | 127));
        const uint64_t net_len = endian::host_to_network(static_cast<uint64_t>(len));
        frame.insert(frame.end(), reinterpret_cast<const byte_t*>(&net_len),
                     reinterpret_cast<const byte_t*>(&net_len) + 8);
    }

    uint32_t masking_key = 0;
    if (masked) {
        thread_local random_mt tl_mt;
        masking_key = tl_mt.next_int<uint32_t>();
        frame.insert(frame.end(), reinterpret_cast<byte_t*>(&masking_key), reinterpret_cast<byte_t*>(&masking_key) + 4);
    }

    if (len > 0) {
        const size_t base = frame.size();
        frame.insert(frame.end(), payload.begin(), payload.end());
        if (masked) {
            const auto* key = reinterpret_cast<const byte_t*>(&masking_key);
            for (size_t i = 0; i < len; ++i) {
                frame[base + i] ^= key[i % 4];
            }
        }
    }

    return frame;
}

string websocket_session_base::make_close_payload(const websocket_status status, const string& reason) {
    const auto code = static_cast<uint16_t>(status);
    string payload;
    payload.reserve(2 + reason.size());
    payload.push_back(static_cast<char>((code >> 8) & 0xFF));
    payload.push_back(static_cast<char>(code & 0xFF));
    payload += reason;
    return payload;
}

NEFORCE_END_NAMESPACE__
