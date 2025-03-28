#include "libwebsockets.h"
#include <WebSocketSample/web_socket_cmds.hpp>
#include <chrono>

namespace skr
{
int _callback_server(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_RECEIVE: {
        // receive data
        auto*  protocol = lws_get_protocol(wsi);
        String data;
        data.reserve(len);
        data.append((const skr_char8*)in, len);
        SKR_LOG_FMT_INFO(u8"圆头<{}>收到： {}", protocol->id, data);

        // make all protocols writable
        lws_callback_on_writable(wsi);
        break;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE: {
        // lws_is_final_fragment(wsi); // 是否是最后一帧
        // lws_frame_is_binary(wsi); // 是否是二进制帧

        // combine data
        auto*           protocol  = lws_get_protocol(wsi);
        String          send_data = format(u8"我是圆头{}, 我要管理", protocol->id);
        Vector<uint8_t> buffer;
        buffer.reserve(
            LWS_SEND_BUFFER_PRE_PADDING +
            LWS_SEND_BUFFER_POST_PADDING +
            send_data.length_buffer()
        );
        buffer.add_zeroed(LWS_SEND_BUFFER_PRE_PADDING);
        buffer.append(send_data.data_raw(), send_data.length_buffer());
        buffer.add_zeroed(LWS_SEND_BUFFER_POST_PADDING);

        // SKR_LOG_FMT_INFO(u8"发送 发送 发送");
        // send
        lws_write(wsi, &buffer.at(LWS_SEND_BUFFER_PRE_PADDING), send_data.length_buffer(), LWS_WRITE_TEXT);

        // lws_is_final_fragment(wsi); // 是否是最后一帧
        // lws_frame_is_binary(wsi); // 是否是二进制帧
        // lws_rx_flow_control(wsi, 1); // 流量控制
        break;
    }
    default:
        // SKR_LOG_FMT_INFO(u8"miss callback：{}", (int32_t)reason);
        break;
    }

    return 0;
}
int _main_server()
{
    // setup protocols
    lws_protocols lws_protocols[3]         = {};
    lws_protocols[0].name                  = "channel-1";
    lws_protocols[0].callback              = _callback_server;
    lws_protocols[0].id                    = 1;
    lws_protocols[0].per_session_data_size = 0;
    lws_protocols[0].rx_buffer_size        = 0;

    lws_protocols[1].name                  = "channel-2";
    lws_protocols[1].callback              = _callback_server;
    lws_protocols[1].id                    = 2;
    lws_protocols[1].per_session_data_size = 0;
    lws_protocols[1].rx_buffer_size        = 0;

    // create context
    lws_context_creation_info lws_ctx_create_info = {};
    lws_ctx_create_info.port                      = 9865;
    lws_ctx_create_info.protocols                 = lws_protocols;
    lws_ctx_create_info.gid                       = -1;
    lws_ctx_create_info.uid                       = -1;
    lws_ctx_create_info.options                   = LWS_SERVER_OPTION_VALIDATE_UTF8;
    lws_context* lws_ctx                          = lws_create_context(&lws_ctx_create_info);
    if (!lws_ctx)
    {
        SKR_LOG_ERROR(u8"圆头创建上下文失败！");
        return -1;
    }

    // loop
    while (true)
    {
        lws_service(lws_ctx, 250);
    }

    lws_context_destroy(lws_ctx);
}
int _callback_client(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_CLIENT_ESTABLISHED: { // success handshake
        SKR_LOG_FMT_INFO(u8"圆头{}连接成功", lws_get_protocol(wsi)->id);
        lws_callback_on_writable(wsi); // make writable
        break;
    }
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        // receive data
        auto*  protocol = lws_get_protocol(wsi);
        String data;
        data.reserve(len);
        data.append((const skr_char8*)in, len);
        SKR_LOG_FMT_INFO(u8"收到圆头<{}>： {}", protocol->id, data);

        // make all protocols writable
        // lws_callback_on_writable_all_protocol(lws_get_context(wsi), lws_get_protocol(wsi));
        break;
    }
    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        // build send data
        auto*           protocol  = lws_get_protocol(wsi);
        String          send_data = format(u8"圆头{}，快干点活！！", protocol->id);
        Vector<uint8_t> buffer;
        buffer.reserve(
            LWS_SEND_BUFFER_PRE_PADDING +
            LWS_SEND_BUFFER_POST_PADDING +
            send_data.length_buffer()
        );
        buffer.add_zeroed(LWS_SEND_BUFFER_PRE_PADDING);
        buffer.append(send_data.data_raw(), send_data.length_buffer());
        buffer.add_zeroed(LWS_SEND_BUFFER_POST_PADDING);

        // send
        lws_write(wsi, &buffer.at(LWS_SEND_BUFFER_PRE_PADDING), send_data.length_buffer(), LWS_WRITE_TEXT);
        break;
    }
    case LWS_CALLBACK_CLOSED: {
        SKR_LOG_FMT_ERROR(u8"圆头{}连接关闭", lws_get_protocol(wsi)->id);
        break;
    }
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
        SKR_LOG_FMT_ERROR(u8"圆头{}连接失败", lws_get_protocol(wsi)->id);
        break;
    }
    }

    return 0;
}
int _main_client()
{
    // setup protocols
    lws_protocols lws_protocols[3]         = {};
    lws_protocols[0].name                  = "channel-1";
    lws_protocols[0].callback              = _callback_client;
    lws_protocols[0].id                    = 1;
    lws_protocols[0].per_session_data_size = 0;
    lws_protocols[0].rx_buffer_size        = 0;

    lws_protocols[1].name                  = "channel-2";
    lws_protocols[1].callback              = _callback_client;
    lws_protocols[1].id                    = 2;
    lws_protocols[1].per_session_data_size = 0;
    lws_protocols[1].rx_buffer_size        = 0;

    // create context
    lws_context_creation_info lws_ctx_create_info = {};
    lws_ctx_create_info.port                      = CONTEXT_PORT_NO_LISTEN;
    lws_ctx_create_info.protocols                 = lws_protocols;
    lws_ctx_create_info.gid                       = -1;
    lws_ctx_create_info.uid                       = -1;
    lws_ctx_create_info.options                   = LWS_SERVER_OPTION_VALIDATE_UTF8;
    lws_context* lws_ctx                          = lws_create_context(&lws_ctx_create_info);

    lws*                                           connect_socket   = nullptr;
    std::chrono::high_resolution_clock::time_point last_try_connect = {};
    std::chrono::high_resolution_clock::time_point last_try_send    = {};
    while (true)
    {
        auto now                 = std::chrono::high_resolution_clock::now();
        auto try_connect_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - last_try_connect).count();
        auto try_send_seconds    = std::chrono::duration_cast<std::chrono::seconds>(now - last_try_send).count();

        // try connect
        if (!connect_socket && try_connect_seconds > 10)
        {
            SKR_LOG_FMT_INFO(u8"尝试连接圆头");
            lws_client_connect_info connect_info = {};
            connect_info.context                 = lws_ctx;
            connect_info.address                 = "127.0.0.1";
            connect_info.port                    = 9865;
            connect_info.path                    = "/";
            connect_info.host                    = "origin";
            connect_info.origin                  = "origin";
            connect_info.protocol                = lws_protocols[0].name;
            connect_socket                       = lws_client_connect_via_info(&connect_info);
            last_try_connect                     = now;
        }

        // check connect
        if (try_send_seconds > 1)
        {
            lws_service(lws_ctx, 250);
            lws_callback_on_writable(connect_socket);
            last_try_send = now;
        }
    }

    lws_context_destroy(lws_ctx);

    return 0;
}

void _custom_log(int level, const char* line)
{
    String temp_log = String::From(line);
    temp_log.remove_at(temp_log.length_buffer() - 1, 1);
    switch (level)
    {
    case LLL_ERR:
        SKR_LOG_ERROR(temp_log.c_str());
        break;
    case LLL_WARN:
        SKR_LOG_WARN(temp_log.c_str());
        break;
    case LLL_NOTICE:
    case LLL_INFO:
        SKR_LOG_INFO(temp_log.c_str());
        break;
    case LLL_DEBUG:
        SKR_LOG_DEBUG(temp_log.c_str());
        break;
    default:
        SKR_LOG_DEBUG(temp_log.c_str());
        break;
    }
}

void WebSocketSampleCli::exec()
{
    lws_set_log_level(LLL_PARSER - 1, _custom_log);

    if (kind == u8"client")
    {
        SKR_LOG_FMT_INFO(u8"圆头客户端开始运行");
        _main_client();
    }
    else if (kind == u8"server")
    {
        SKR_LOG_FMT_INFO(u8"圆头服务端开始运行");
        _main_server();
    }
    else
    {
        SKR_LOG_FMT_ERROR(u8"圆头不支持的类型：{}", kind);
    }
}
} // namespace skr