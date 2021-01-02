#include <logging/logging.h>

namespace logging
{
std::unique_ptr<manual_timer> factory::create_manual_timer(const std::string& name)
{
    return std::make_unique<manual_timer>(name);
}
std::unique_ptr<scoped_timer> factory::create_scoped_timer(const std::string& name)
{
    return std::make_unique<scoped_timer>(name);
}
}// namespace logging
