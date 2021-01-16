#pragma once
#include <string>
#include <exception>

namespace pfaedle::gtfs
{
struct invalid_field_format : public std::exception
{
public:
    explicit invalid_field_format(const std::string & msg) : message(prefix + msg) {}

    const char * what() const noexcept { return message.c_str(); }

private:
    const std::string prefix = "Invalid GTFS field format. ";
    std::string message;
};
}
