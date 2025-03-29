#include "SkrV8/v8_inspector.hpp"
#include "SkrV8/v8_context.hpp"
#include "v8-platform.h"
#include "libplatform/libplatform.h"
#include "SkrV8/v8_isolate.hpp"

namespace skr
{
V8WebSocketServer::V8WebSocketServer() = default;
V8WebSocketServer::~V8WebSocketServer()
{
    shutdown();
    for (auto* buffer : _pending_package)
    {
        SkrDelete(buffer);
    }
    _pending_package.clear();

    for (auto* buffer : _buffer_pool)
    {
        SkrDelete(buffer);
    }
    _buffer_pool.clear();
}

// init & shutdown
bool V8WebSocketServer::init(int port)
{
    shutdown();

    _port = port;

    // vhost
    _pvo_default.next    = nullptr;
    _pvo_default.options = nullptr;
    _pvo_default.name    = "default";
    _pvo_default.value   = "1";
    _pvo.next            = &_pvo_default;
    _pvo.options         = nullptr;
    _pvo.name            = "inspect";
    _pvo.value           = "";

    // fill protocol
    _proto[0].name     = "http"; // support document
    _proto[0].user     = this;
    _proto[0].callback = _callback_server;
    _proto[1].name     = "inspect";
    _proto[1].user     = this;
    _proto[1].callback = _callback_server;

    // create context
    lws_context_creation_info create_info{};
    create_info.port      = port;
    create_info.protocols = _proto;
    create_info.pvo       = &_pvo;
    create_info.gid       = -1;
    create_info.uid       = -1;
    create_info.user      = this;
    create_info.options   = LWS_SERVER_OPTION_VALIDATE_UTF8; // accept utf8
    _ctx                  = lws_create_context(&create_info);
    if (!_ctx)
    {
        SKR_LOG_FMT_ERROR(u8"failed to create websocket server on port '{}'", port);
        return false;
    }

    _combine_json_response();

    return true;
}
void V8WebSocketServer::shutdown()
{
    if (_ctx)
    {
        lws_context_destroy(_ctx);
        _ctx = nullptr;
    }
}

// message
void V8WebSocketServer::send_message(StringView message)
{
    // push package
    auto* buffer = _alloc_buffer();
    buffer->reserve(
        LWS_SEND_BUFFER_PRE_PADDING +
        LWS_SEND_BUFFER_POST_PADDING +
        message.length_buffer()
    );
    buffer->add_zeroed(LWS_SEND_BUFFER_PRE_PADDING);
    buffer->append(message.data_raw(), message.length_buffer());
    buffer->add_zeroed(LWS_SEND_BUFFER_POST_PADDING);
    _pending_package.push_back(buffer);

    // send message
    lws_callback_on_writable_all_protocol(_ctx, _proto);
}
void V8WebSocketServer::send_http_response(StringView message)
{
    // push message
    auto* buffer = _alloc_buffer();
    buffer->reserve(message.length_buffer());
    buffer->append(message.data_raw(), message.length_buffer());
    _pending_http_package.push_back(buffer);

    // send message
    lws_callback_on_writable_all_protocol(_ctx, _proto);
}
void V8WebSocketServer::pump_messages()
{
    if (!_pending_package.is_empty())
    {
        lws_callback_on_writable_all_protocol(_ctx, _proto);
    }
    lws_service(_ctx, 0);
}

// callback
int V8WebSocketServer::_callback_server(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len)
{
    auto* self = reinterpret_cast<V8WebSocketServer*>(lws_context_user(lws_get_context(wsi)));
    switch (reason)
    {
    case LWS_CALLBACK_HTTP: {
        String url = (const char8_t*)in;

        // SKR_LOG_FMT_DEBUG(u8"receive http request: {}", url.c_str());
        if (url == u8"/json/version")
        {
            self->send_http_response(u8"HTTP/1.0 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nCache-Control: no-cache\r\n\r\n");
            self->send_http_response(self->_json_version);
        }
        else if (url == u8"/json/list")
        {
            self->send_http_response(u8"HTTP/1.0 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nCache-Control: no-cache\r\n\r\n");
            self->send_http_response(self->_json_list);
        }
        else
        {
            lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, nullptr);
        }
        break;
    }
    case LWS_CALLBACK_HTTP_WRITEABLE: {
        auto* buffer = self->_pending_http_package.pop_front_get();
        lws_write(
            wsi,
            buffer->data(),
            buffer->size(),
            LWS_WRITE_HTTP
        );
        self->_free_buffer(buffer);

        if (!self->_pending_http_package.is_empty())
        {
            lws_callback_on_writable(wsi);
        }
        break;
    }
    case LWS_CALLBACK_RECEIVE: {
        // receive data
        String data;
        data.reserve(len);
        data.append((const skr_char8*)in, len);
        self->on_message_received.invoke(data);
        SKR_LOG_FMT_DEBUG(u8"receive message: {}", data.c_str());
        break;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE: {
        auto* buffer = self->_pending_package.pop_front_get();
        lws_write(
            wsi,
            &buffer->at(LWS_SEND_BUFFER_PRE_PADDING),
            buffer->size() - LWS_SEND_BUFFER_PRE_PADDING - LWS_SEND_BUFFER_POST_PADDING,
            LWS_WRITE_TEXT
        );
        self->_free_buffer(buffer);
        break;
    }
    default:
        // SKR_LOG_FMT_DEBUG(u8"lost message: {}", (int32_t)reason);
        break;
    }

    return 0;
}

// json response
void V8WebSocketServer::_combine_json_response()
{
    _json_version.clear();
    _json_version.append(u8R"__({
        "Browser": "SakuraEngine",
        "Protocol-Version": "1.1"
    })__");

    _json_list.clear();
    _json_list.append(u8"[{");
    _json_list.append(u8R"("description": "SakuraEngine Inspector",)");
    _json_list.append(u8R"("id": "0",)");
    _json_list.append(u8R"("title": "SakuraEngine Inspector",)");
    _json_list.append(u8R"("type": "node",)");
    _json_list.append(format(u8R"("webSocketDebuggerUrl": "ws://127.0.0.1:{}")", _port));
    _json_list.append(u8"}]");
}

// override
void V8InspectorChannel::sendResponse(int callId, std::unique_ptr<v8_inspector::StringBuffer> message)
{
    String message_str;
    if (message->string().is8Bit())
    {
        message_str = String::From((const char*)message->string().characters8(), message->string().length());
    }
    else
    {
        message_str = String::From((const wchar_t*)message->string().characters16(), message->string().length());
    }

    server->send_message(message_str);
}
void V8InspectorChannel::sendNotification(std::unique_ptr<v8_inspector::StringBuffer> message)
{
    String message_str;
    if (message->string().is8Bit())
    {
        message_str = String::From((const char*)message->string().characters8(), message->string().length());
    }
    else
    {
        message_str = String::From((const wchar_t*)message->string().characters16(), message->string().length());
    }

    server->send_message(message_str);
}
void V8InspectorChannel::flushProtocolNotifications()
{
    // do nothing
}

// init & shutdown
void V8InspectorClient::init(V8Isolate* isolate)
{
    _isolate = isolate;

    // pass server
    _channel.server = server;

    // create v8 data
    _inspector = v8_inspector::V8Inspector::create(isolate->v8_isolate(), this);
    _session   = _inspector->connect(
        kContextGroupId,
        &_channel,
        v8_inspector::StringView{ (const uint8_t*)kContextName.data(), kContextName.size() },
        v8_inspector::V8Inspector::ClientTrustLevel::kFullyTrusted,
        v8_inspector::V8Inspector::SessionPauseState::kWaitingForDebugger
    );

    // register callback
    _on_message_received_handle = server->on_message_received.bind_lambda(
        [this](StringView message) {
            _session->dispatchProtocolMessage(
                { (const uint8_t*)message.data_raw(),
                  message.length_buffer() }
            );
        }
    );
}
void V8InspectorClient::shutdown()
{
    // unregister callback
    server->on_message_received.remove(_on_message_received_handle);

    _inspector.reset();
    _session.reset();
}

// override
void V8InspectorClient::runMessageLoopOnPause(int contextGroupId)
{
    if (_is_runing_message_loop) { return; }

    _is_runing_message_loop = true;

    while (_run_message_loop)
    {
        _isolate->pump_message_loop();
    }

    _is_runing_message_loop = false;
}
void V8InspectorClient::quitMessageLoopOnPause()
{
    _run_message_loop = false;
}
void V8InspectorClient::runIfWaitingForDebugger(int contextGroupId)
{
    _connected = true;
}

// notify
void V8InspectorClient::notify_context_created(V8Context* context, StringView name)
{
    v8::HandleScope handle_scope(_isolate->v8_isolate());

    v8_inspector::StringView name_view{ (const uint8_t*)name.data_raw(), name.length_buffer() };

    v8_inspector::V8ContextInfo info{
        context->v8_context().Get(_isolate->v8_isolate()),
        kContextGroupId,
        name_view,
    };
    _inspector->contextCreated(info);
}

// debug control
void V8InspectorClient::pause_on_next_statement(StringView reason)
{
    v8_inspector::StringView reason_view{ (const uint8_t*)reason.data_raw(), reason.length_buffer() };
    _session->schedulePauseOnNextStatement(reason_view, reason_view);
}
void V8InspectorClient::run_message_loop_on_next_pause()
{
    _run_message_loop = true;
}

} // namespace skr