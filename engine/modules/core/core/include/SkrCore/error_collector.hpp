#pragma once
#include <SkrCore/log.hpp>
#include <SkrContainers/vector.hpp>
#include <source_location>

namespace skr
{
struct ErrorCollector {
    enum class ELevel
    {
        Fault,
        Error,
        Warning,
        kCount,
    };

    struct StackScope {
        StackScope(ErrorCollector* logger, String name)
            : _logger(logger)
        {
            _logger->_stack.push_back({ name, {} });
        }
        ~StackScope()
        {
            auto old_stack = _logger->_stack.pop_back_get();

            // update any_log
            if (!_logger->_stack.is_empty())
            {
                for (size_t i = 0; i < (size_t)ELevel::kCount; ++i)
                {
                    _logger->_stack.back().any_log[i] |= old_stack.any_log[i];
                }
            }
        }

    private:
        ErrorCollector* _logger;
    };

    template <typename... Args>
    inline StackScope stack(StringView fmt, Args&&... args)
    {
        return StackScope{
            this,
            format(fmt, std::forward<Args>(args)...)
        };
    }
    inline StackScope stack_source(std::source_location loc = std::source_location::current())
    {
        return stack(u8"{} [{}:{}]", loc.function_name(), loc.file_name(), loc.line());
    }

    // log
    template <typename... Args>
    inline void log(ELevel level, StringView fmt, Args&&... args)
    {
        // record log status
        {
            auto& stack                  = _stack.is_empty() ? _root_stack : _stack.back();
            stack.any_log[(size_t)level] = true;
        }

        // combine log str
        String log;
        format_to(log, fmt, std::forward<Args>(args)...);
        log.append(u8"\n");
        for (const auto& stack : _stack.range_inv())
        {
            format_to(log, u8"    At: {}\n", stack.name);
        }
        log.first(log.length_buffer() - 1); // remove last '\n'

        // do log
        switch (level)
        {
        case ELevel::Fault:
            SKR_LOG_FMT_FATAL(u8"{}", log.c_str());
            break;
        case ELevel::Error:
            SKR_LOG_FMT_ERROR(u8"{}", log.c_str());
            break;
        case ELevel::Warning:
            SKR_LOG_FMT_WARN(u8"{}", log.c_str());
            break;
        default:
            SKR_UNREACHABLE_CODE()
            break;
        }
    }
    template <typename... Args>
    inline void fault(StringView fmt, Args&&... args)
    {
        log(ELevel::Fault, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    inline void error(StringView fmt, Args&&... args)
    {
        log(ELevel::Error, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    inline void warning(StringView fmt, Args&&... args)
    {
        log(ELevel::Warning, fmt, std::forward<Args>(args)...);
    }

    // any log
    inline bool any_log(ELevel level) const
    {
        SKR_ASSERT(level < ELevel::kCount);
        auto& stack = _stack.is_empty() ? _root_stack : _stack.back();
        return stack.any_log[(size_t)level];
    }
    inline bool any_log() const
    {
        for (size_t i = 0; i < (size_t)ELevel::kCount; ++i)
        {
            if (any_log((ELevel)i)) return true;
        }
        return false;
    }
    inline bool any_fault() const { return any_log(ELevel::Fault); }
    inline bool any_error() const { return any_log(ELevel::Error); }
    inline bool any_warning() const { return any_log(ELevel::Warning); }

private:
    struct ErrorStack {
        String name                            = {};
        bool   any_log[(size_t)ELevel::kCount] = {};
    };
    ErrorStack         _root_stack = {};
    Vector<ErrorStack> _stack;
};
} // namespace skr