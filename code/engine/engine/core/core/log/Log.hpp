#pragma once

#include "core/pch.hpp"

#ifdef DEBUG
    #include <spdlog/spdlog.h>

namespace zong
{
namespace core
{

/// <summary>
/// two loggers has different name, one is for core, and another is for editor
/// </summary>
class Log
{
private:
    static std::shared_ptr<spdlog::logger> _coreLogger;
    static std::shared_ptr<spdlog::logger> _clientLogger;

public:
    static const std::shared_ptr<spdlog::logger>& getCoreLogger();
    static const std::shared_ptr<spdlog::logger>& getClientLogger();

public:
    /// <summary>
    /// init loggers with two names, and set log filename
    /// </summary>
    static void init();
};

} // namespace core
} // namespace zong

    /**
     * @defgroup Log Log.h debug macro
     * @{core log macros: \n
     * * ZONG_CORE_TRACE: basic information
     * * ZONG_CORE_INFO: need be optimized
     * * ZONG_CORE_WARN: should be solved
     * * ZONG_CORE_ERROR: error
     * * ZONG_CORE_CRITICAL: serious error
     * * ZONG_CORE_ASSERT: assert
     *
     * client log macros: \n
     * * ZONG_TRACE: basic information
     * * ZONG_INFO: need be optimized
     * * ZONG_WARN: should be solved
     * * ZONG_ERROR: error
     * * ZONG_CRITICAL: serious error
     * * ZONG_ASSERT: assert
     */

    // core log macros
    #define ZONG_CORE_TRACE(...) zong::core::Log::getCoreLogger()->trace(__VA_ARGS__)
    #define ZONG_CORE_INFO(...) zong::core::Log::getCoreLogger()->info(__VA_ARGS__)
    #define ZONG_CORE_WARN(...) zong::core::Log::getCoreLogger()->warn(__VA_ARGS__)
    #define ZONG_CORE_ERROR(...) zong::core::Log::getCoreLogger()->error(__VA_ARGS__)
    #define ZONG_CORE_CRITICAL(...) zong::core::Log::getCoreLogger()->critical(__VA_ARGS__)
    #define ZONG_CORE_ASSERT(x, ...)                                   \
        {                                                              \
            if (!(x))                                                  \
            {                                                          \
                ZONG_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); \
                __debugbreak();                                        \
            }                                                          \
        }

    // client log macros
    #define ZONG_TRACE(...) zong::core::Log::getClientLogger()->trace(__VA_ARGS__)
    #define ZONG_INFO(...) zong::core::Log::getClientLogger()->info(__VA_ARGS__)
    #define ZONG_WARN(...) zong::core::Log::getClientLogger()->warn(__VA_ARGS__)
    #define ZONG_ERROR(...) zong::core::Log::getClientLogger()->error(__VA_ARGS__)
    #define ZONG_CRITICAL(...) zong::core::Log::getClientLogger()->critical(__VA_ARGS__)
    #define ZONG_ASSERT(x, ...)                                   \
        {                                                         \
            if (!(x))                                             \
            {                                                     \
                ZONG_ERROR("Assertion Failed: {0}", __VA_ARGS__); \
                __debugbreak();                                   \
            }                                                     \
        }
#elif RELEASE
// core log macros
    #define ZONG_CORE_TRACE(...)
    #define ZONG_CORE_INFO(...)
    #define ZONG_CORE_WARN(...)
    #define ZONG_CORE_ERROR(...)
    #define ZONG_CORE_CRITICAL(...)
    #define ZONG_CORE_ASSERT(x, ...)

    // client log macros
    #define ZONG_TRACE(...)
    #define ZONG_INFO(...)
    #define ZONG_WARN(...)
    #define ZONG_ERROR(...)
    #define ZONG_CRITICAL(...)
    #define ZONG_ASSERT(x, ...)
#endif

/** @} */