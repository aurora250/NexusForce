#include <NeForce/network/http/grpc.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/memory/endian.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

byte_vector grpc_framer::encode(const grpc_message& msg) {
    byte_vector frame;
    frame.push_back(msg.compressed ? 1 : 0);

    auto len = static_cast<uint32_t>(msg.payload.size());
    frame.push_back(static_cast<byte_t>((len >> 24) & 0xFF));
    frame.push_back(static_cast<byte_t>((len >> 16) & 0xFF));
    frame.push_back(static_cast<byte_t>((len >> 8) & 0xFF));
    frame.push_back(static_cast<byte_t>(len & 0xFF));

    frame.insert(frame.end(), msg.payload.begin(), msg.payload.end());
    return frame;
}

byte_vector grpc_framer::encode_messages(const vector<grpc_message>& messages) {
    byte_vector result;
    for (const auto& msg: messages) {
        auto frame = encode(msg);
        result.insert(result.end(), frame.begin(), frame.end());
    }
    return result;
}

int grpc_framer::decode(const byte_t* data, size_t len, vector<grpc_message>& out) {
    size_t offset = 0;
    int count = 0;

    while (offset + 5 <= len) {
        bool compressed = data[offset] != 0;
        offset++;

        uint32_t msg_len = endian::read_be32(data + offset);
        offset += 4;

        if (msg_len > max_receive_size.bytes()) {
            return -1;
        }

        if (offset + msg_len > len) {
            // frame incomplete
            break;
        }

        grpc_message msg;
        msg.compressed = compressed;
        msg.payload.assign(data + offset, data + offset + msg_len);
        out.push_back(move(msg));

        offset += msg_len;
        count++;
    }

    return count;
}

void grpc_handler::process_unary(http_request& request, http_response& response, unary_handler handler) {
    vector<grpc_message> messages;
    const auto* data = reinterpret_cast<const byte_t*>(request.body.data());
    int n = framer_.decode(data, request.body.size(), messages);

    if (n < 0) {
        send_error(response, grpc_status::INVALID_ARGUMENT, "Invalid gRPC frame");
        return;
    }
    if (n == 0 || messages.empty()) {
        send_error(response, grpc_status::INVALID_ARGUMENT, "Empty gRPC request");
        return;
    }

    try {
        auto result = handler(messages[0]);

        response.status = http_status::S2_OK;
        response.status_message = "OK";
        response.set_content_type(GRPC_CONTENT_TYPE);
        response.body.assign(reinterpret_cast<const char*>(result.payload.data()), result.payload.size());

        auto encoded = framer_.encode(result);
        response.body.assign(reinterpret_cast<const char*>(encoded.data()), encoded.size());

        // set gRPC trailer
        response.trailers["grpc-status"] = "0";
        response.trailers["grpc-message"] = "";
    } catch (const exception& e) {
        send_error(response, grpc_status::INTERNAL, e.what());
    }
}

void grpc_handler::send_error(http_response& response, grpc_status status, const string& message) {
    response.status = grpc_to_http_status(status);
    response.status_message = message;
    response.set_content_type(GRPC_CONTENT_TYPE);
    response.trailers["grpc-status"] = to_string(static_cast<int>(status));
    response.trailers["grpc-message"] = message;
}

http_status grpc_handler::grpc_to_http_status(grpc_status status) noexcept {
    switch (status) {
        case grpc_status::OK:
            return http_status::S2_OK;
        case grpc_status::INVALID_ARGUMENT:
            return http_status::S4_BAD_REQUEST;
        case grpc_status::NOT_FOUND:
            return http_status::S4_NOT_FOUND;
        case grpc_status::PERMISSION_DENIED:
            return http_status::S4_FORBIDDEN;
        case grpc_status::UNAUTHENTICATED:
            return http_status::S4_UNAUTHORIZED;
        case grpc_status::UNIMPLEMENTED:
            return http_status::S5_NOT_IMPLEMENTED;
        case grpc_status::UNAVAILABLE:
            return http_status::S5_SERVICE_UNAVAILABLE;
        case grpc_status::DEADLINE_EXCEEDED:
            return http_status::S4_REQUEST_TIMEOUT;
        case grpc_status::RESOURCE_EXHAUSTED:
            return http_status::S4_TOO_MANY_REQUESTS;
        case grpc_status::ABORTED:
            return http_status::S4_CONFLICT;
        case grpc_status::OUT_OF_RANGE:
            return http_status::S4_BAD_REQUEST;
        case grpc_status::INTERNAL:
        default:
            return http_status::S5_INTERNAL_SERVER_ERROR;
    }
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
