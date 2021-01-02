#include "logging/logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/syslog_sink.h>
#include <memory>
#include <utility>

namespace logging
{

log::log(logging::log_level level, source_loc&& location) :
    level_{level},
    message_{},
    location_{location}
{
}

log::log(std::string  name, logging::log_level level, source_loc&& location) :
        name_{std::move(name)},
        level_{level},
        message_{},
        location_{location}
{}


log::~log()
{
    auto default_logger = spdlog::default_logger_raw();
    std::string to;
    while (std::getline(message_, to, '\n'))
    {
        default_logger->log(spdlog::source_loc{location_.filename_, location_.line_, location_.funcname_}, static_cast<spdlog::level::level_enum>(level_), to);
    }
}
void log::log_message(const std::stringstream& message)
{
    message_ << message.str();
}

void configure_logging()
{
    // add console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("%H:%M:%S %^%l%$ [tid %t] %s:%# %v");

    // add file sink
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/logs_main.txt", 0, 0, true);
    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[%H:%M:%S %z] [%n] [%l] [thread %t] %s:%# %v");

    auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>("pfaedle", 1, 1, true);
    syslog_sink->set_level(spdlog::level::warn);

    auto logger = std::shared_ptr<spdlog::logger>(new spdlog::logger("main", {console_sink, file_sink, syslog_sink}));
    logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);
}
void add_logger(const std::string& name)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    console_sink->set_pattern("[%^%l%$] %v");

    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/logs_" + name + ".txt", 0, 0, true);
    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[%H:%M:%S %z] [%n] [%l] [thread %t] %s:%# %v");

    auto& sinks = spdlog::get(name)->sinks();
    sinks.push_back(console_sink);
    sinks.push_back(file_sink);
}

}// namespace logging
