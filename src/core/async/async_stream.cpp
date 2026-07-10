#include <NeForce/core/async/async_stream.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct sg_read_state : enable_shared_from_this<sg_read_state> {
        async_stream* self;
        io_context* ctx;
        mutable_buffers* bufs;
        size_t index{0};
        size_t offset{0};
        size_t total{0};
        function<void(error_code, size_t)> handler;

        void start() { do_read(); }

        void do_read() {
            if (index >= bufs->size()) {
                handler(error_code{}, total);
                return;
            }
            const memory_view<char> view((*bufs)[index].data() + offset, (*bufs)[index].size() - offset);
            auto keep_alive = shared_from_this();
            self->async_read(*ctx, view, [keep_alive](error_code ec, size_t n) { keep_alive->on_read(ec, n); });
        }

        void on_read(error_code ec, size_t n) {
            if (ec) {
                handler(ec, total);
                return;
            }
            total += n;
            offset += n;
            if (offset >= (*bufs)[index].size()) {
                ++index;
                offset = 0;
            }
            if (n == 0 || index >= bufs->size()) {
                handler(error_code{}, total);
            } else {
                do_read();
            }
        }
    };

    struct sg_write_state : enable_shared_from_this<sg_write_state> {
        async_stream* self;
        io_context* ctx;
        const_buffers* bufs;
        size_t index{0};
        size_t offset{0};
        size_t total{0};
        function<void(error_code, size_t)> handler;

        void start() { do_write(); }

        void do_write() {
            if (index >= bufs->size()) {
                handler(error_code{}, total);
                return;
            }
            const memory_view<const char> view((*bufs)[index].data() + offset, (*bufs)[index].size() - offset);
            auto keep_alive = shared_from_this();
            self->async_write(*ctx, view, [keep_alive](error_code ec, size_t n) { keep_alive->on_write(ec, n); });
        }

        void on_write(error_code ec, size_t n) {
            if (ec) {
                handler(ec, total);
                return;
            }
            total += n;
            offset += n;
            if (offset >= (*bufs)[index].size()) {
                ++index;
                offset = 0;
            }
            if (index >= bufs->size()) {
                handler(error_code{}, total);
            } else {
                do_write();
            }
        }
    };
} // namespace


void async_stream::async_read(io_context& ctx, mutable_buffers& bufs, function<void(error_code, size_t)> handler) {
    const auto state = make_shared<sg_read_state>();
    state->self = this;
    state->ctx = &ctx;
    state->bufs = &bufs;
    state->handler = move(handler);
    state->start();
}

void async_stream::async_write(io_context& ctx, const_buffers& bufs, function<void(error_code, size_t)> handler) {
    const auto state = make_shared<sg_write_state>();
    state->self = this;
    state->ctx = &ctx;
    state->bufs = &bufs;
    state->handler = move(handler);
    state->start();
}

NEFORCE_END_NAMESPACE__
