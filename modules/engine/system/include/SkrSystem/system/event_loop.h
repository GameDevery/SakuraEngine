#pragma once
#include "SkrSystem/event.h"

#ifdef __cplusplus

namespace skr {

struct ISystemEventQueue;
struct ISystemEventHandler;

struct SKR_SYSTEM_API ISystemEventSource
{
    virtual ~ISystemEventSource() SKR_NOEXCEPT;

    virtual bool poll_event(SkrSystemEvent& event) SKR_NOEXCEPT = 0;
};

struct SKR_SYSTEM_API ISystemEventQueue
{
    static ISystemEventQueue* Create() SKR_NOEXCEPT;
    static void Free(ISystemEventQueue* queue) SKR_NOEXCEPT;

    virtual ~ISystemEventQueue() SKR_NOEXCEPT;

    virtual bool pump_messages() SKR_NOEXCEPT = 0;
    virtual bool add_source(ISystemEventSource* source) SKR_NOEXCEPT = 0;
    virtual bool add_handler(ISystemEventHandler* handler) SKR_NOEXCEPT = 0;
};

struct SKR_SYSTEM_API ISystemEventHandler
{
    virtual ~ISystemEventHandler() SKR_NOEXCEPT;
    virtual void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT = 0;
};

} // namespace skr

#endif