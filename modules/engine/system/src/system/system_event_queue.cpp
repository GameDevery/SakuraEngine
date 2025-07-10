#include "SkrSystem/system_app.h"
#include "SkrContainers/vector.hpp"
#include "SkrCore/log.h"

namespace skr {

struct SystemEventQueue::Impl
{
    skr::Vector<ISystemEventSource*> sources;
    skr::Vector<ISystemEventHandler*> handlers;
};

SystemEventQueue::SystemEventQueue() SKR_NOEXCEPT
    : pimpl(new Impl())
{
}

SystemEventQueue::~SystemEventQueue() SKR_NOEXCEPT
{
    delete pimpl;
}

SystemEventQueue::SystemEventQueue(SystemEventQueue&& other) SKR_NOEXCEPT
    : pimpl(other.pimpl)
{
    other.pimpl = nullptr;
}

SystemEventQueue& SystemEventQueue::operator=(SystemEventQueue&& other) SKR_NOEXCEPT
{
    if (this != &other)
    {
        delete pimpl;
        pimpl = other.pimpl;
        other.pimpl = nullptr;
    }
    return *this;
}

bool SystemEventQueue::pump_messages() SKR_NOEXCEPT
{
    if (!pimpl || pimpl->handlers.is_empty())
        return false;
        
    bool has_events = false;
    
    // Poll and dispatch events directly
    for (auto* source : pimpl->sources)
    {
        SkrSystemEvent event;
        while (source->poll_event(event))
        {
            has_events = true;
            
            // Dispatch to all handlers immediately
            for (auto* handler : pimpl->handlers)
            {
                handler->handle_event(event);
            }
        }
    }
    
    return has_events;
}

bool SystemEventQueue::add_source(ISystemEventSource* source) SKR_NOEXCEPT
{
    if (!pimpl || !source)
        return false;
        
    // Check for duplicates using find
    if (pimpl->sources.find(source))
        return false;
    
    pimpl->sources.add(source);
    return true;
}

bool SystemEventQueue::remove_source(ISystemEventSource* source) SKR_NOEXCEPT
{
    if (!pimpl || !source)
        return false;
        
    // Use find to locate the source
    if (auto ref = pimpl->sources.find(source))
    {
        pimpl->sources.remove_at(ref.index());
        return true;
    }
    return false;
}

bool SystemEventQueue::add_handler(ISystemEventHandler* handler) SKR_NOEXCEPT
{
    if (!pimpl || !handler)
        return false;
        
    // Check for duplicates using find
    if (pimpl->handlers.find(handler))
        return false;
    
    pimpl->handlers.add(handler);
    return true;
}

bool SystemEventQueue::remove_handler(ISystemEventHandler* handler) SKR_NOEXCEPT
{
    if (!pimpl || !handler)
        return false;
        
    // Use find to locate the handler
    if (auto ref = pimpl->handlers.find(handler))
    {
        pimpl->handlers.remove_at(ref.index());
        return true;
    }
    return false;
}

void SystemEventQueue::clear_sources() SKR_NOEXCEPT
{
    if (pimpl)
    {
        pimpl->sources.clear();
    }
}

void SystemEventQueue::clear_handlers() SKR_NOEXCEPT
{
    if (pimpl)
    {
        pimpl->handlers.clear();
    }
}

// Virtual destructor implementations for interfaces that still need them
ISystemEventSource::~ISystemEventSource() SKR_NOEXCEPT = default;
ISystemEventHandler::~ISystemEventHandler() SKR_NOEXCEPT = default;

} // namespace skr