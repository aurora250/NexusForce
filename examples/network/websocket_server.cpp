/**
 * @example websocket_server.cpp
 * @brief WebSocket服务器示例（含permessage-deflate压缩）
 *
 * 演示通过 http_server 使用WebSocket功能：
 * - WebSocket路由注册
 * - permessage-deflate 消息压缩（RFC 7692）
 * - 文本/二进制消息收发
 * - 心跳检测（Ping/Pong）
 * - 广播消息
 * - 关闭处理
 *
 * 测试方法：
 *   启动后打开浏览器控制台，执行：
 *   let ws = new WebSocket("ws://localhost:8080/chat");
 *   ws.onmessage = (e) => console.log("Received:", e.data);
 *   ws.send("Hello NexusForce!");
 *
 *   或使用 wscat（支持deflate协商）：
 *   wscat -c ws://localhost:8080/chat
 */

#include <NeForce/core/async/event_loop.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/websocket_deflate.hpp>

using namespace neforce;
using namespace neforce::http;

int main() {
    http_server server(ports(8080u));

    // =========================================================================
    // 首页（含压缩状态显示）
    // =========================================================================

    server.router().get("/", [](http_request& req, http_response& res) {
        res.body = R"(<!DOCTYPE html>
<html>
<head><title>NexusForce WebSocket Demo</title></head>
<body>
<h1>WebSocket Chat Demo</h1>
<p>permessage-deflate compression: <span id="deflate" style="color:orange">checking...</span></p>
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
    var deflate = (ws.extensions && ws.extensions.indexOf('permessage-deflate') >= 0);
    document.getElementById('deflate').textContent = deflate ? 'active' : 'inactive';
    document.getElementById('deflate').style.color = deflate ? 'green' : 'orange';
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

    // =========================================================================
    // WebSocket路由（含permessage-deflate压缩）
    // =========================================================================

    server.websocket().route("/chat", [](websocket_server::session_ptr session) {
        println("New WebSocket connection");

        // 检查deflate协商结果
        if (session->has_deflate_config() && session->deflate_config().active) {
            printfln("  permessage-deflate enabled:"
                     " client_wbits={}, server_wbits={},"
                     " client_noctx={}, server_noctx={}",
                     session->deflate_config().client_max_window_bits, session->deflate_config().server_max_window_bits,
                     session->deflate_config().client_no_context_takeover,
                     session->deflate_config().server_no_context_takeover);
        } else {
            println("  permessage-deflate: not negotiated (client didn't request)");
        }

        // 消息处理器
        session->set_message_handler([session](const string& msg, websocket_opcode opcode) {
            if (opcode == websocket_opcode::TEXT) {
                printfln("Received text ({} bytes): {}", msg.size(), msg);
                // Echo消息
                session->send("Echo: " + msg);
            } else if (opcode == websocket_opcode::BINARY) {
                printfln("Received binary data ({} bytes)", msg.size());
                session->send("Binary received: " + to_string(msg.size()) + " bytes");
            }
        });

        // 关闭处理器
        session->set_close_handler([](websocket_status status, const string& reason) {
            printfln("WebSocket closed: code={}, reason={}", static_cast<uint16_t>(status), reason);
        });

        // 错误处理器
        session->set_error_handler([](const exception& e) { printfln("WebSocket error: {}", e.what()); });

        // 启动会话（开始接收消息）
        session->start();

        // 发送欢迎消息
        string welcome = "Welcome to NexusForce WebSocket Chat!";
        if (session->has_deflate_config() && session->deflate_config().active) {
            welcome += " (deflate enabled)";
        }
        session->send(welcome);
    });

    // 使用 event_loop 驱动 WebSocket I/O（替代多线程模式）
    event_loop ws_loop;
    server.websocket().set_event_loop(&ws_loop);

    // 启动服务器
    if (server.start()) {
        thread ws_loop_thread([&ws_loop] { ws_loop.run(); });
        printfln("WebSocket Server started on http://localhost:{}", static_cast<uint16_t>(server.port()));
        println("Open your browser and navigate to the address above.");
        printfln("Or use wscat: wscat -c ws://localhost:{}/chat", static_cast<uint16_t>(server.port()));
        println("permessage-deflate is automatically negotiated when client requests it.");
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
