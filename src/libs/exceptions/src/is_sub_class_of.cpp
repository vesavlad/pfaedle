#include "is_sub_class_of.h"

#include <cxxabi.h>

namespace
{
#ifndef __clang__

/**
 * Try to match target type with any of the base types of a given type.
 * The implementation underneath is based on learnings from following:
 * https://github.com/CPTI/Exception/blob/master/project/src/Exception.cpp
 * https://monoinfinito.wordpress.com/series/exception-handling-in-c/
 */
bool find_base(
        const abi::__class_type_info* currentType,
        const std::type_info& targetType)
{
    if (currentType)
    {
        if (*currentType == targetType)
        {/// When currentType and targetType refer to the same type, we found our target as a base type of the starting type
            return true;
        }
        else if (const auto* siType = dynamic_cast<const abi::__si_class_type_info*>(currentType))
        {/// currentType is a single inheritance class type
            return find_base(siType->__base_type, targetType);
        }
        else if (const auto* vmType = dynamic_cast<const abi::__vmi_class_type_info*>(currentType))
        {/// currentType is a virtual / multiple inheritance class type
            unsigned int n = 0;
            for (; n < vmType->__base_count && !find_base(vmType->__base_info[n].__base_type, targetType); ++n)
                ;
            return (n < vmType->__base_count);
        }
    }
    return false;
}
#endif// __clang__
}// namespace

namespace exceptions
{
bool is_sub_class_of(
        const std::type_info& actual_type [[maybe_unused]],
        const std::type_info& base_type [[maybe_unused]])
{
#ifndef __clang__
    /// actualType needs to be a class type, it could be the type info of a primitive, function pointer, enum class, etc.
    const auto* classAbiType = dynamic_cast<const abi::__class_type_info*>(&actual_type);
    return (classAbiType && find_base(classAbiType, base_type));
#else
    return false;
#endif// __clang__
}

std::string demangle(const std::type_info& info)
{
    int status = 0;
    char* name = abi::__cxa_demangle(info.name(), nullptr, nullptr, &status);// NOLINT
    std::string result(name);
    free(name);// NOLINT
    return result;
}

}// namespace exceptions
