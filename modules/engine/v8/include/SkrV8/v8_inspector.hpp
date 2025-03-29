#pragma once
#include "SkrBase/config.h"
#include "SkrContainersDef/ring_buffer.hpp"
#include "SkrCore/delegate/event.hpp"
#include "v8-inspector.h"
#include "SkrV8/v8_isolate.hpp"
#include <libwebsockets.h>

namespace skr
{
// websocket
struct SKR_V8_API V8WebSocketServer {
    V8WebSocketServer();
    ~V8WebSocketServer();

    // init & shutdown
    bool init(int port);
    void shutdown();

    // message
    void send_message(StringView message);
    void send_http_response(StringView message);
    void pump_messages();

public:
    // receive callback
    Event<void(StringView)> on_message_received;

private:
    // pooling api
    inline Vector<uint8_t>* _alloc_buffer()
    {
        if (_buffer_pool.size() > 0)
        {
            return _buffer_pool.pop_back_get();
        }
        else
        {
            return SkrNew<Vector<uint8_t>>();
        }
    }
    inline void _free_buffer(Vector<uint8_t>* buffer)
    {
        buffer->clear();
        _buffer_pool.push_back(buffer);
    }

    // callback
    static int _callback_server(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len);

    // json response
    void _combine_json_response();

private:
    lws_context*                 _ctx                  = nullptr;
    lws_protocols                _proto[3]             = {};
    lws_protocol_vhost_options   _pvo                  = {};
    lws_protocol_vhost_options   _pvo_default          = {};
    RingBuffer<Vector<uint8_t>*> _pending_package      = {};
    RingBuffer<Vector<uint8_t>*> _pending_http_package = {};
    Vector<Vector<uint8_t>*>     _buffer_pool          = {};
    int                          _port                 = 0;

    // receive
    String _json_version;
    String _json_list;
};

// channel
struct SKR_V8_API V8InspectorChannel : v8_inspector::V8Inspector::Channel {
    // override
    void sendResponse(int callId, std::unique_ptr<v8_inspector::StringBuffer> message) override;
    void sendNotification(std::unique_ptr<v8_inspector::StringBuffer> message) override;
    void flushProtocolNotifications() override;

    V8WebSocketServer* server = nullptr;
};

// inspector client
struct SKR_V8_API V8InspectorClient : v8_inspector::V8InspectorClient {
    // init & shutdown
    void init(V8Isolate* isolate);
    void shutdown();

    // override
    void runMessageLoopOnPause(int contextGroupId) override;
    void quitMessageLoopOnPause() override;
    void runIfWaitingForDebugger(int contextGroupId) override;

    // notify
    void notify_context_created(V8Context* context, StringView name);

    // getter
    inline v8_inspector::V8InspectorSession*       v8_session() { return _session.get(); }
    inline const v8_inspector::V8InspectorSession* v8_session() const { return _session.get(); }
    inline v8_inspector::V8Inspector*              v8_inspector() { return _inspector.get(); }
    inline const v8_inspector::V8Inspector*        v8_inspector() const { return _inspector.get(); }
    inline bool                                    is_connected() const { return _connected; }

    // debug control
    void pause_on_next_statement(StringView reason);
    void run_message_loop_on_next_pause();

public:
    static constexpr int        kContextGroupId = 1;
    static constexpr StringView kContextName    = u8"test_context";

    V8WebSocketServer* server = nullptr;

private:
    // handle for listen server
    EventBindID _on_message_received_handle = 0;

    // waiting info
    bool _connected = false;

    // inspect env
    V8Isolate* _isolate = nullptr;

    // v8 component
    std::unique_ptr<v8_inspector::V8Inspector>        _inspector;
    std::unique_ptr<v8_inspector::V8InspectorSession> _session;
    V8InspectorChannel                                _channel;

    // message loop
    bool _is_runing_message_loop = false;
    bool _run_message_loop       = false;
};

} // namespace skr