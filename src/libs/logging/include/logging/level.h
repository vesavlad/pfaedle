#ifndef LOGGING_LEVEL_H
#define LOGGING_LEVEL_H

#define TRACE 0
#define DEBUG 1
#define INFO 2
#define WARN 3
#define ERROR 4
#define CRITICAL 5
#define OFF 6

namespace logging
{
enum log_level
{
    trace = TRACE,
    debug = DEBUG,
    info = INFO,
    warn = WARN,
    error = ERROR,
    critical = CRITICAL,
    off = OFF,
    n_levels
};

}// namespace logging
#endif//LOGGING_LEVEL_H
