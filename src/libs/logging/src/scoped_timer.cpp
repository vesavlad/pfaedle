#include <logging/logger.h>
#include <logging/scoped_timer.h>

namespace logging
{
scoped_timer::scoped_timer(std::string name) :
    name_{std::move(name)},
    start_{std::chrono::steady_clock::now()}
{
    LOG(info) << "[" << name_ << "] starting";
}

scoped_timer::~scoped_timer()
{
    using namespace std::chrono;
    auto stop = steady_clock::now();
    double t = duration_cast<microseconds>(stop - start_).count() / 1000.0;
    LOG(info) << "[" << name_ << "] finished"
              << " (" << t << "ms)";
}

}// namespace logging
