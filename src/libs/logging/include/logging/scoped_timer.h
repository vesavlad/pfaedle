#ifndef LOGGING_SCOPED_TIMER_H
#define LOGGING_SCOPED_TIMER_H

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>

namespace logging
{
class scoped_timer final
{
public:
    explicit scoped_timer(std::string name);
    scoped_timer(scoped_timer const&) = delete;
    scoped_timer(scoped_timer&&) = delete;
    scoped_timer& operator=(scoped_timer const&) = delete;
    scoped_timer& operator=(scoped_timer&&) = delete;
    ~scoped_timer();

private:
    std::string name_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
};
}// namespace logging
#endif//LOGGING_SCOPED_TIMER_H
