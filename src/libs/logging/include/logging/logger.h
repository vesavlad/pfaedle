#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

#include <logging/level.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <mutex>
#include <string>

namespace logging
{

struct source_loc
{
    constexpr source_loc() = default;
    constexpr source_loc(const char *filename_in, int line_in, const char *funcname_in)
            : filename_{filename_in}
            , line_{line_in}
            , funcname_{funcname_in}
    {}

    constexpr bool empty() const noexcept
    {
        return line_ == 0;
    }
    const char *filename_{nullptr};
    int line_{0};
    const char *funcname_{nullptr};
};

class log
{
public:
    log(logging::log_level level, source_loc&& location);
    log(std::string  name, logging::log_level level, source_loc&& location);

    log(log const&) = delete;
    log& operator=(log const&) = delete;

    log(log&&) = default;
    log& operator=(log&&) = default;

    template<typename T>
    friend log&& operator<<(log&& l, T&& t)
    {
        std::stringstream stream;
        stream << t;

        l.log_message(stream);

        return std::move(l);
    }


    ~log();

private:
    void log_message(const std::stringstream& message);

    std::string name_;
    logging::log_level level_;
    std::stringstream message_;
    source_loc location_;
};


void configure_logging();

void add_logger(const std::string& name);

}// namespace logging

#define LOG(lvl) logging::log(static_cast<logging::log_level>(lvl), logging::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)})
#define LOGF(name, lvl) logging::log(name, lvl, logging::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)})

#define LOG_TRACE() LOG(TRACE)
#define LOG_DEBUG() LOG(DEBUG)
#define LOG_INFO() LOG(INFO)
#define LOG_WARN() LOG(WARN)
#define LOG_ERROR() LOG(ERROR)
#define LOG_CRITICAL() LOG(CRITICAL)
#define LOG_OFF() LOG(OFF)

#define LOGF_TRACE(name) LOGF(name, TRACE)
#define LOGF_DEBUG(name) LOGF(name, DEBUG)
#define LOGF_INFO(name) LOGF(name, INFO)
#define LOGF_WARN(name) LOGF(name, WARN)
#define LOGF_ERROR(name) LOGF(name, ERROR)
#define LOGF_CRITICAL(name) LOGF(name, CRITICAL)
#define LOGF_OFF(name) LOGF(name, OFF)

#endif//LOGGING_LOGGER_H
