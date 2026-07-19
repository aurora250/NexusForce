#include "ws_broadcaster.hpp"

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/serialize/json_serializer.hpp>
#include <NeForce/logging/logger.hpp>

using namespace neforce;
using namespace neforce::http;

void WsBroadcaster::add_session(websocket_server::session_ptr session) {
    lock<mutex> lk(mtx_);
    sessions_.push_back(session);
}

void WsBroadcaster::broadcast(const string& event, const Task& task) {
    reflect::meta_any obj(task);
    auto json_str = serialize::json_serializer::serialize(obj);

    lock<mutex> lk(mtx_);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if ((*it)->is_open()) {
            (*it)->send(json_str.get()->to_string());
            ++it;
        } else {
            it = sessions_.erase(it);
        }
    }

    NEFORCE_LOGGER_LOGF_INFO("app.task", "WebSocket 广播: event={}, id={}", event, task.id);
}
