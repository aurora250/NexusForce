#include <NeForce/network/http/grpc.hpp>
#include <NeForce/network/http/http2_connection.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/memory/endian.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

byte_vector grpc_framer::encode(const grpc_message& msg) {
    byte_vector frame;
    frame.push_back(msg.compressed ? 1 : 0);

    const auto len = static_cast<uint32_t>(msg.payload.size());
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
        const bool compressed = data[offset] != 0;
        offset++;

        const uint32_t msg_len = endian::read_be32(data + offset);
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
    const int n = framer_.decode(data, request.body.size(), messages);

    if (n < 0) {
        send_error(response, grpc_status::invalid_argument, "Invalid gRPC frame");
        return;
    }
    if (n == 0 || messages.empty()) {
        send_error(response, grpc_status::invalid_argument, "Empty gRPC request");
        return;
    }

    try {
        auto result = handler(messages[0]);

        response.status = http_status::ok;
        response.status_message = "OK";
        response.set_content_type(GRPC_CONTENT_TYPE);
        response.body.assign(reinterpret_cast<const char*>(result.payload.data()), result.payload.size());

        auto encoded = framer_.encode(result);
        response.body.assign(reinterpret_cast<const char*>(encoded.data()), encoded.size());

        // set gRPC trailer
        response.trailers["grpc-status"] = "0";
        response.trailers["grpc-message"] = "";
    } catch (const exception& e) {
        send_error(response, grpc_status::internal, e.what());
    }
}

void grpc_stream_writer::start() {
    if (started_ || finished_) {
        return;
    }
    vector<hpack_header_field> headers;
    headers.push_back({":status", "200"});
    headers.push_back({"content-type", string(GRPC_CONTENT_TYPE)});
    connection_.send_headers(stream_id_, headers, false);
    started_ = true;
}

bool grpc_stream_writer::write(const grpc_message& message) {
    if (finished_) {
        return false;
    }
    if (!started_) {
        start();
    }
    const auto frame = grpc_framer::encode(message);
    connection_.send_data(stream_id_, string_view(reinterpret_cast<const char*>(frame.data()), frame.size()), false);
    return true;
}

void grpc_stream_writer::finish(const grpc_status status, const string& message) {
    if (finished_) {
        return;
    }
    if (!started_) {
        start();
    }
    vector<hpack_header_field> trailers;
    trailers.push_back({"grpc-status", to_string(static_cast<int>(status))});
    trailers.push_back({"grpc-message", message});
    connection_.send_trailers(stream_id_, trailers);
    finished_ = true;
}

void grpc_handler::process_server_stream(http2_connection& connection, const uint32_t stream_id, const byte_t* data,
                                         const size_t len, const stream_handler& handler) {
    grpc_stream_writer writer(connection, stream_id);
    vector<grpc_message> messages;
    grpc_framer framer;
    const int decoded = framer.decode(data, len, messages);
    if (decoded <= 0 || messages.empty()) {
        writer.start();
        writer.finish(grpc_status::invalid_argument, "Empty gRPC request");
        return;
    }
    writer.start();
    try {
        handler(messages.front(),
                [&writer](const grpc_message& response) { static_cast<void>(writer.write(response)); });
        writer.finish(grpc_status::ok);
    } catch (const exception& e) {
        writer.finish(grpc_status::internal, e.what());
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
        case grpc_status::ok:
            return http_status::ok;
        case grpc_status::invalid_argument:
            return http_status::bad_request;
        case grpc_status::not_found:
            return http_status::not_found;
        case grpc_status::permission_denied:
            return http_status::forbidden;
        case grpc_status::unauthenticated:
            return http_status::unauthorized;
        case grpc_status::unimplemented:
            return http_status::not_implemented;
        case grpc_status::unavailable:
            return http_status::service_unavailable;
        case grpc_status::deadline_exceeded:
            return http_status::request_timeout;
        case grpc_status::resource_exhausted:
            return http_status::too_many_requests;
        case grpc_status::aborted:
            return http_status::conflict;
        case grpc_status::out_of_range:
            return http_status::bad_request;
        case grpc_status::internal:
        default:
            return http_status::internal_server_error;
    }
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
