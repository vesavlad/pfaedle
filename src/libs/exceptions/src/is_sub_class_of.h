#ifndef EXCEPTIONS_IS_SUBCLASS_OF_H
#define EXCEPTIONS_IS_SUBCLASS_OF_H

#include <string>
#include <typeinfo>

namespace exceptions
{

#ifdef __GNUC__
bool is_sub_class_of(
        const std::type_info& actual_type,
        const std::type_info& base_type);
#endif// __GNUC__

std::string demangle(
        const std::type_info& info);

}// namespace exceptions

#endif// EXCEPTIONS_IS_SUBCLASS_OF_H
