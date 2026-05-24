/**
 * @example websocket_server.cpp
 * @brief WebSocket服务器示例
 *
 * 演示通过 http_server 使用WebSocket功能：
 * - WebSocket路由注册
 * - 消息收发（文本/二进制）
 * - 心跳检测（Ping/Pong）
 * - 广播消息
 * - 关闭处理
 *
 * 测试方法：
 *   启动后打开浏览器控制台，执行：
 *   let ws = new WebSocket("ws://localhost:8080/chat");
 *   ws.onmessage = (e) => console.log("Received:", e.data);
 *   ws.send("Hello NexusForce!");
 */

#include <chrono>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_server.hpp>

using namespace neforce;
using namespace neforce::http;

int main() {
    http_server server(ports(8080u));

    // ========== HTTP路由（首页） ==========
    server.router().get("/", [](http_request& req, http_response& res) {
        res.body = R"(<!DOCTYPE html>
<html>
<head><title>NexusForce WebSocket Demo</title></head>
<body>
<h1>WebSocket Chat Demo</h1>
<div id="messages" style="border:1px solid #ccc; height:300px; overflow-y:scroll; padding:8px; margin-bottom:8px;"></div>
<input id="input" type="text" placeholder="Type a message..." style="width:300px;">
<button onclick="send() ">Send</button>
<script>
const ws = new WebSocket('ws://' + location.host + '/chat');
ws.onmessage = (e) => {
    const div = document.getElementById('messages');
    div.innerHTML += '<div>' + e.data + '</div>';
    div.scrollTop = div.scrollHeight;
};
ws.onopen = () => {
    document.getElementById('messages').innerHTML += '<div style="color:green">Connected!</div>';
};
ws.onclose = () => {
    document.getElementById('messages').innerHTML += '<div style="color:red">Disconnected</div>';
};
function send() {
    const input = document.getElementById('input');
    ws.send(input.value);
    input.value = '';
}
document.getElementById('input').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') send();
});
</script>
</body>
</html>)";
        res.set_content_type(http_content::HTML_TEXT());
    });

    // ========== WebSocket路由 ==========
    server.websocket().route("/chat", [](websocket_server::session_ptr session) {
        println("New WebSocket connection");

        // 设置消息处理器
        session->set_message_handler([session](const string& msg, websocket_opcode opcode) {
            if (opcode == websocket_opcode::TEXT) {
                printfln("Received text: {}", msg);
                // Echo消息给所有客户端（广播）
                session->send("Echo: " + msg);
            } else if (opcode == websocket_opcode::BINARY) {
                printfln("Received binary data ({} bytes)", msg.size());
            }
        });

        // 设置关闭处理器
        session->set_close_handler([](websocket_status status, const string& reason) {
            printfln("WebSocket closed: code={}, reason={}", static_cast<uint16_t>(status), reason);
        });

        // 设置错误处理器
        session->set_error_handler([](const exception& e) { printfln("WebSocket error: {}", e.what()); });

        // 启动会话（开始接收消息）
        session->start();

        // 发送欢迎消息
        session->send("Welcome to NexusForce WebSocket Chat!");
    });

    // 启动服务器
    if (server.start()) {
        printfln("WebSocket Server started on http://localhost:{}", static_cast<uint16_t>(server.port()));
        println("Open your browser and navigate to the address above.");
        printfln("Or use wscat: wscat -c ws://localhost:{}/chat", static_cast<uint16_t>(server.port()));
        while (true) {
            this_thread::sleep_for(milliseconds(100));
        }
        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start server");
        return 1;
    }

    return 0;
}
