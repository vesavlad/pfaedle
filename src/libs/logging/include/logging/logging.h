#pragma once

#include <logging/logger.h>
#include <logging/manual_timer.h>
#include <logging/scoped_timer.h>

#include <memory>
namespace logging
{
class factory
{
public:
    static std::unique_ptr<manual_timer> create_manual_timer(const std::string& name);
    static std::unique_ptr<scoped_timer> create_scoped_timer(const std::string& name);
};
}// namespace logging
