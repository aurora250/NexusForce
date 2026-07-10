#include <NeForce/core/async/async_compose.hpp>
#include <NeForce/core/memory/shared_ptr.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct read_state : enable_shared_from_this<read_state> {
        async_stream* stream;
        io_context* ctx;
        vector<char>* buffer;
        size_t offset{0};
        size_t total{0};
        function<void(error_code, size_t)> handler;

        void start() {
            total = buffer->size();
            do_read();
        }

        void do_read() {
            auto self = shared_from_this();
            const memory_view<char> view(buffer->data() + offset, total - offset);
            stream->async_read(*ctx, view, [self](const error_code ec, const size_t n) { self->on_read(ec, n); });
        }

        void on_read(const error_code ec, const size_t n) {
            if (ec) {
                handler(ec, offset);
                return;
            }
            offset += n;
            if (offset >= total || n == 0) {
                handler(error_code{}, offset);
            } else {
                do_read();
            }
        }
    };

    struct write_state : enable_shared_from_this<write_state> {
        async_stream* stream;
        io_context* ctx;
        const char* data;
        size_t offset{0};
        size_t total{0};
        function<void(error_code, size_t)> handler;

        void start() { do_write(); }

        void do_write() {
            auto self = shared_from_this();
            const memory_view<const char> view(data + offset, total - offset);
            stream->async_write(*ctx, view, [self](const error_code ec, const size_t n) { self->on_write(ec, n); });
        }

        void on_write(const error_code ec, const size_t n) {
            if (ec) {
                handler(ec, offset);
                return;
            }
            offset += n;
            if (offset >= total) {
                handler(error_code{}, offset);
            } else {
                do_write();
            }
        }
    };
} // namespace


void async_read(async_stream& stream, io_context& ctx, vector<char>& buffer,
                function<void(error_code, size_t)> handler) {
    const auto state = make_shared<read_state>();
    state->stream = &stream;
    state->ctx = &ctx;
    state->buffer = &buffer;
    state->handler = move(handler);
    state->start();
}

void async_write(async_stream& stream, io_context& ctx, const void* data, const size_t size,
                 function<void(error_code, size_t)> handler) {
    const auto state = make_shared<write_state>();
    state->stream = &stream;
    state->ctx = &ctx;
    state->data = static_cast<const char*>(data);
    state->total = size;
    state->handler = move(handler);
    state->start();
}

NEFORCE_END_NAMESPACE__
