#include <exceptions/exceptions.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "stacktrace.h"
#pragma GCC diagnostic pop
#include <sstream>

namespace
{
void add_message(std::ostringstream& oss, const std::string& message, const std::string& file, int line)
{
    oss << message << " @" << file << ":" << line;
}
}// namespace

std::string make_message(const std::string& message, const std::string& file, int line)
{
    std::ostringstream oss;
    add_message(oss, message, file, line);
    return oss.str();
}

std::string make_message_with_call_stack(const std::string& message)
{
    std::ostringstream oss;
    oss << message;
    oss << std::endl
        << "callstack :" << std::endl;
    oss << markusjx::stacktrace::stacktrace();
    return oss.str();
}

std::string make_message_with_call_stack(const std::string& message, const std::string& file, int line)
{
    std::ostringstream oss;
    add_message(oss, message, file, line);
    oss << std::endl
        << "callstack :" << std::endl;
    oss << markusjx::stacktrace::stacktrace();
    return oss.str();
}
