#include "exception_handler.h"
#include "exception_trace.h"
#include "is_sub_class_of.h"

#include <exceptions/exceptions.h>
#include <logging/logger.h>

#include <dlfcn.h>
#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

std::string build_throw_message(const std::type_info* type_info, void* thrown_exception)
{
    if (type_info)
    {
        /// Check 'typeInfo' to determine if 'thrown_exception' is an std::exception, if so reuse the exception's message
        if (exceptions::is_sub_class_of(*type_info, typeid(std::exception)))
        {
            std::string message = "Exception of type '" + exceptions::demangle(*type_info) + "' thrown\n";
            const std::string what = static_cast<const std::exception*>(thrown_exception)->what();

            return message + (what.find("callstack :", 0, 11) == std::string::npos ? make_message_with_call_stack(what) : what);
        }
        return make_message_with_call_stack("Instance of type '" + exceptions::demangle(*type_info) + "' thrown");
    }
    return make_message_with_call_stack("Instance of unknown type thrown");
}

#if 0

/**
 * We're replacing here a function from the standard library by our own implementation.
 * Overloading such a function is done by passing a function with the same signature to the linker,
 * but earlier than in the list of libraries to be linked (stdlib is not mentioned explicitly, so comes last).
 * This function is not in a namespace (otherwise the original one would not be overloaded).
 */
typedef void (*cxa_throw_type)(void *, void *, void (*) (void *));
cxa_throw_type orig_cxa_throw = nullptr;

void load_orig_throw_code()
{
    orig_cxa_throw = (cxa_throw_type) dlsym(RTLD_NEXT, "__cxa_throw");
}

extern "C"
void __cxa_throw(void* thrown_exception, std::type_info* pvtinfo, void (*dest)(void*))// NOLINT
{
    LOG(logging::error) << build_throw_message(static_cast<const std::type_info*>(pvtinfo), thrown_exception);

    if (orig_cxa_throw == nullptr)
        load_orig_throw_code();

    /// Pass the exception to the standard exception handling mechanism, by calling __cxa_throw from stdlib
    orig_cxa_throw(thrown_exception, pvtinfo, dest);
}
#endif

void my_terminate()
{
    std::cerr << "terminate handler called\n";

    std::exception_ptr eptr = std::current_exception();

    try
    {
        std::rethrow_exception(eptr);
    }
    catch (std::exception& exc)
    {
        exceptions::exception_trace::exception(exc);
        LOG(logging::error) << std::string(exc.what());
    }
    catch (...)
    {
        exceptions::exception_trace::message("caught something undefined");
        LOG(logging::error) << "caught something undefined";
    }

    std::abort();// forces abnormal termination
}

int install_uncaught_exception_logger()
{
    std::set_terminate(my_terminate);
    return 0;
}
namespace exceptions
{
exception_handler::exception_handler() :
    dummy_for_uncaught_exceptions_(install_uncaught_exception_logger())

{
}

}// namespace exceptions
