#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>

/**
   create a message
   @param[in] Message : the exception message
   @param[in] file : file in which the exception occurred
   @param[in] line : line number at which the exception occurred
   @return: A string containing message, file name and line number
*/
std::string make_message(const std::string& Message, const std::string& file, int line);

/**
   create an exception
   @param[in] Message : the exception message
   @param[in] file : file in which the exception occurred
   @param[in] line : line number at which the exception occurred
   @return: An exception object of type T containing a message, with appended file name and line number
*/
template<typename T>
T make_exception(const std::string& Message, const std::string& file, int line)
{
    return T(make_message(Message, file, line));
}

/**
   create a message with a callstack
   @param[in] Message : the exception message
   @return: A string containing message and callstack
*/
std::string make_message_with_call_stack(const std::string& Message);

/**
   create a message with a callstack
   @param[in] Message : the exception message
   @param[in] file : file in which the exception occurred
   @param[in] line : line number at which the exception occurred
   @return: A string containing message, file name, line number and callstack
*/
std::string make_message_with_call_stack(const std::string& Message, const std::string& file, int line);

/**
   create an exception with a callstack
   @param[in] Message : the exception message
   @param[in] file : file in which the exception occurred
   @param[in] line : line number at which the exception occurred
   @return: An exception object of type T containing message, file name, line number and callstack
*/
template<typename T>
T make_exception_with_call_stack(const std::string& Message, const std::string& file, int line)
{
    return T(make_message_with_call_stack(Message, file, line));
}

/**
    Convenience macro for MakeExceptionWithCallStack
*/
#define make_exception_with_call_stack_macro(T, x) make_exception_with_call_stack<T>(x, __FILE__, __LINE__)

/**
    Convenience macro for MakeException
*/
#define make_exception_macro(T, x) make_exception<T>(x, __FILE__, __LINE__)

/**
   Undefined exception class
*/
class undefined_exception : public std::runtime_error
{
public:
    /**
       Constructor
       @param[in] message : the exception message
    */
    undefined_exception(const std::string& message) :
        std::runtime_error(message) {}
};

/**
   Invalid parameter exception class
*/
class invalid_parameter_exception : public std::runtime_error
{
public:
    /**
       Constructor
       @param[in] message : the exception message
    */
    invalid_parameter_exception(const std::string& message) :
        std::runtime_error(message) {}
};

/**
   Null pointer exception class
*/
class null_pointer_exception : public std::runtime_error
{
public:
    /**
       Constructor
       @param[in] message : the exception message
    */
    null_pointer_exception(const std::string& message) :
        std::runtime_error(message) {}
};


#endif//EXCEPTIONS_H
