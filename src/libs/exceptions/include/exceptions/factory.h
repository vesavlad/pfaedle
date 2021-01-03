#ifndef MOTILIUM_FACTORY_H
#define MOTILIUM_FACTORY_H

#include <memory>

namespace exceptions
{
class exception_handler_if;
class factory
{
public:
    static std::unique_ptr<exception_handler_if> create_exceptions_handler();
};
}  // namespace exceptions
#endif//MOTILIUM_FACTORY_H
