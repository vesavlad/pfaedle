#pragma once
#include <pfaedle/gtfs/types.h>
#include <pfaedle/gtfs/access/result_code.h>

namespace pfaedle::gtfs::access
{
struct result
{
    result() = default;
    result(result_code && in_code) : code(in_code) {}
    result(const result_code & in_code, const Message & msg) : code(in_code), message(msg) {}
    bool operator==(result_code result_code) const { return code == result_code; }
    bool operator!=(result_code result_code) const { return !(*this == result_code); }

    result_code code = OK;
    Message message;
};

}
