#include <logging/logger.h>
#include <logging/manual_timer.h>

namespace logging
{
manual_timer::manual_timer(std::string name) :
    name_{std::move(name)},
    start_{std::chrono::steady_clock::now()}
{
    LOG(info) << "[" << name_ << "] starting";
}

void manual_timer::stop_and_print()
{
    using namespace std::chrono;
    auto stop = steady_clock::now();
    double t = duration_cast<microseconds>(stop - start_).count() / 1000.0;
    LOG(info) << "[" << name_ << "] finished"
              << " (" << t << "ms)";
}
}// namespace logging
