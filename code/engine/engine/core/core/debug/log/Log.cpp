#include "Log.h"

#ifdef DEBUG
    #include <spdlog/sinks/basic_file_sink.h>
    #include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> zong::core::Log::_coreLogger   = nullptr;
std::shared_ptr<spdlog::logger> zong::core::Log::_clientLogger = nullptr;

const std::shared_ptr<spdlog::logger>& zong::core::Log::coreLogger()
{
    if (!_coreLogger)
        init();
    return Log::_coreLogger;
}

const std::shared_ptr<spdlog::logger>& zong::core::Log::clientLogger()
{
    if (!_clientLogger)
        init();
    return Log::_clientLogger;
}

void zong::core::Log::init()
{
    if (_coreLogger || _clientLogger)
    {
        ZONG_CORE_INFO("the log has been init");
        return;
    }

    std::vector<spdlog::sink_ptr> logSinks;
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/Engine.log", true));

    logSinks[0]->set_pattern("%^[%T] %n: %v%$");
    logSinks[1]->set_pattern("[%T] [%l] %n: %v");

    _coreLogger = std::make_shared<spdlog::logger>("Engine", begin(logSinks), end(logSinks));
    spdlog::register_logger(_coreLogger);
    _coreLogger->set_level(spdlog::level::trace);
    _coreLogger->flush_on(spdlog::level::trace);

    _clientLogger = std::make_shared<spdlog::logger>("Editor", begin(logSinks), end(logSinks));
    spdlog::register_logger(_clientLogger);
    Log::_clientLogger->set_level(spdlog::level::trace);
    Log::_clientLogger->flush_on(spdlog::level::trace);
}

#endif
