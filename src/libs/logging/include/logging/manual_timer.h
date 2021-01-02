#ifndef LOGGING_MANUAL_TIMER_H
#define LOGGING_MANUAL_TIMER_H

#include <chrono>
#include <string>

namespace logging
{
class manual_timer final
{
public:
    explicit manual_timer(std::string name);
    void stop_and_print();

private:
    std::string name_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
};
}// namespace logging

#endif//LOGGING_MANUAL_TIMER_H
