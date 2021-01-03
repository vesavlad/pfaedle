#ifndef EXCEPTIONS_EXCEPTIONTRACE_H
#define EXCEPTIONS_EXCEPTIONTRACE_H

#include <stdexcept>
#include <string>

namespace exceptions::exception_trace
{
void message(const std::string& message_in);
void exception(const std::exception& exception_in);

void install_crash_handlers(const std::string& storage);
void register_rip_file(const std::string& name);
void unregister_rip_file(const std::string& name);

}// namespace exceptions::exception_trace

#endif// EXCEPTIONS_EXCEPTIONTRACE_H
