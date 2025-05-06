#pragma once
#include "../common/io_request.hpp"
#include "SkrGraphics/dstorage.h"

namespace skr
{
namespace io
{

struct SKR_RUNTIME_API DStorageEvent : public skr::IRCAble {
    SKR_RC_IMPL();

public:
    void skr_rc_delete() override
    {
        pool->deallocate(this);
    }

    bool okay() { return skr_dstorage_event_test(event); }

    friend struct SmartPool<DStorageEvent, DStorageEvent>;

protected:
    DStorageEvent(ISmartPoolPtr<DStorageEvent> pool, SkrDStorageQueueId queue)
        : queue(queue)
        , pool(pool)
    {
        if (!event)
            event = skr_dstorage_queue_create_event(queue);
    }
    ~DStorageEvent() SKR_NOEXCEPT
    {
        if (event)
        {
            skr_dstorage_queue_free_event(queue, event);
        }
        queue = nullptr;
    }
    friend struct DStorageRAMReader;
    friend struct DStorageVRAMReader;
    skr::InlineVector<IOBatchId, 32> batches;
    SkrDStorageQueueId               queue = nullptr;
    ISmartPoolPtr<DStorageEvent>     pool  = nullptr;
    SkrDStorageEventId               event = nullptr;
};

} // namespace io
} // namespace skr