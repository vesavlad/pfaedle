#pragma once
#include <gtfs/types.h>
#include <gtfs/access/result_code.h>

#include <utility>

namespace pfaedle::gtfs::access
{
struct result
{
    result() = default;
    result(result_code && in_code) : code(in_code) {} //NOLINT
    result(const result_code & in_code, Message msg) : code(in_code), message(std::move(msg)) {}
    bool operator==(result_code result_code) const { return code == result_code; }
    bool operator!=(result_code result_code) const { return !(*this == result_code); }

    result_code code = OK;
    Message message;
};

}
