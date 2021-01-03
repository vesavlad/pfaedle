#include <exceptions/factory.h>

#include "exception_handler.h"
#include "exception_trace.h"

namespace exceptions
{
std::unique_ptr<exception_handler_if> factory::create_exceptions_handler()
{
    exception_trace::install_crash_handlers(".");
    return std::unique_ptr<exception_handler_if>{new exception_handler()};
}
}  // namespace exceptions
