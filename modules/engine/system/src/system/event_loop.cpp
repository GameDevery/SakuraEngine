#include "SkrSystem/system/event_loop.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrCore/memory/memory.h"

namespace skr {

class SystemEventQueue;

ISystemEventSource::~ISystemEventSource() SKR_NOEXCEPT
{

}

ISystemEventHandler::~ISystemEventHandler() SKR_NOEXCEPT
{

}

ISystemEventQueue::~ISystemEventQueue() SKR_NOEXCEPT
{

}

class SystemEventQueue : public ISystemEventQueue
{
public:
    SystemEventQueue() SKR_NOEXCEPT = default;
    ~SystemEventQueue() SKR_NOEXCEPT override = default;

    bool pump_messages() SKR_NOEXCEPT override
    {
        bool has_events = false;
        
        for (auto* source : sources)
        {
            SkrSystemEvent e;
            while (source->poll_event(e))
            {
                has_events = true;
                
                // 获取到事件后，立即分发给所有处理器
                for (auto* handler : handlers)
                {
                    handler->handle_event(e);
                }
            }
        }
        
        return has_events;
    }

    bool add_source(ISystemEventSource* source) SKR_NOEXCEPT override
    {
        if (!source || sources.contains(source))
            return false;
        
        sources.add(source);
        return true;
    }

    bool add_handler(ISystemEventHandler* handler) SKR_NOEXCEPT override
    {
        if (!handler || handlers.contains(handler)) 
            return false;
        
        handlers.add(handler);
        return true;
    }

private:
    skr::Vector<ISystemEventSource*> sources;
    skr::Vector<ISystemEventHandler*> handlers;
};

ISystemEventQueue* ISystemEventQueue::Create() SKR_NOEXCEPT
{
    return SkrNew<SystemEventQueue>();
}

void ISystemEventQueue::Free(ISystemEventQueue* queue) SKR_NOEXCEPT
{
    if (queue)
    {
        SkrDelete(queue);
    }
}

} // namespace skr