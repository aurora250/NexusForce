/**
 * @brief WebSocket 广播管理器——向所有连接的客户端推送任务变更事件
 */

#ifndef INTEGRATION_WS_BROADCASTER_HPP__
#define INTEGRATION_WS_BROADCASTER_HPP__

#include "task.hpp"

#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/network/http/websocket.hpp>

class WsBroadcaster {
public:
    /// 注册新会话
    void add_session(neforce::http::websocket_server::session_ptr session);

    /// 广播任务变更事件
    void broadcast(const neforce::string& event, const Task& task);

private:
    neforce::vector<neforce::http::websocket_server::session_ptr> sessions_;
    neforce::mutex mtx_;
};

#endif
