#include "SkrCore/async/named_thread.hpp"
#include "SkrCore/async/wait_timeout.hpp"
#include "SkrCore/log.h"

namespace skr
{
NamedThreadFunction::~NamedThreadFunction() SKR_NOEXCEPT
{
}

NamedThread::NamedThread() SKR_NOEXCEPT
    : started(false),
      alive(false),
      func(nullptr)
{
}

NamedThread::~NamedThread() SKR_NOEXCEPT
{
    finalize();
}

void NamedThread::threadFunc(void* args)
{
    NamedThread* pSelf = (NamedThread*)args;
    skr_current_thread_set_name(pSelf->tname.c_str());
    pSelf->tID = skr_current_thread_id();

    skr_atomic_store_release(&pSelf->started, true);
    skr_atomic_store_release(&pSelf->alive, true);

    pSelf->func->run();

    skr_atomic_store_release(&pSelf->alive, false);
}

AsyncResult NamedThread::start(NamedThreadFunction* pFunc) SKR_NOEXCEPT
{
    if (skr_atomic_load_acquire(&started))
    {
        SKR_LOG_ERROR(u8"thread is already started!");
        return ASYNC_RESULT_ERROR_THREAD_ALREADY_STARTES;
    }

    this->func = pFunc;

    tDesc.pFunc = &threadFunc;
    tDesc.pData = this;
    skr_init_thread(&tDesc, &tHandle);

    const auto P = (SThreadPriority)skr_atomic_load_acquire(&priority);
    skr_thread_set_priority(tHandle, P);

    wait_timeout([this]() { return skr_atomic_load_acquire(&started); }, 4);

    return ASYNC_RESULT_OK;
}

AsyncResult NamedThread::join() SKR_NOEXCEPT
{
    skr_join_thread(tHandle);
    return ASYNC_RESULT_OK;
}

bool NamedThread::is_alive() const SKR_NOEXCEPT
{
    return skr_atomic_load_acquire(&alive);
}

bool NamedThread::has_started() const SKR_NOEXCEPT
{
    return skr_atomic_load_acquire(&started);
}

SThreadID NamedThread::get_id() const SKR_NOEXCEPT
{
    return tID;
}

SThreadPriority NamedThread::change_priority(SThreadPriority pri) SKR_NOEXCEPT
{
    skr_atomic_store_relaxed(&priority, pri);
    return skr_thread_set_priority(tHandle, pri);
}

NamedThreadFunction* NamedThread::get_function() const SKR_NOEXCEPT
{
    return func;
}

AsyncResult NamedThread::initialize(const NamedThreadDesc& pdesc) SKR_NOEXCEPT
{
    desc  = pdesc;
    tname = skr::String::From(desc.name ? desc.name : u8"unnamed");
    skr_atomic_store_relaxed(&priority, priority);
    return ASYNC_RESULT_OK;
}

AsyncResult NamedThread::finalize() SKR_NOEXCEPT
{
    if (skr_atomic_load_acquire(&started))
    {
        skr_destroy_thread(tHandle);
        skr_atomic_store_release(&started, false);
    }
    return ASYNC_RESULT_OK;
}

} // namespace skr