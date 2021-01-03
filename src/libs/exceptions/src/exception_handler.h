#ifndef EXCEPTIONS_THROWLOG_H
#define EXCEPTIONS_THROWLOG_H

#include <exceptions/exception_handler_if.h>
namespace exceptions
{
class exception_handler : public exception_handler_if
{
public:
    exception_handler();
    exception_handler(exception_handler const&) = delete;
    exception_handler& operator=(exception_handler const&) = delete;

    exception_handler(exception_handler&&) = delete;
    exception_handler& operator=(exception_handler&&) = delete;

    ~exception_handler() override = default;

private:
    int dummy_for_uncaught_exceptions_;
};
}// namespace exceptions


#endif//EXCEPTIONS_THROWLOG_H
