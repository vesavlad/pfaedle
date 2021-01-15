#pragma once
#include <map>
#include <string>
#include <variant>

namespace pfaedle::gtfs
{
class feed;
struct record
{
    using payload_type = std::map<std::string, std::variant<std::string, double, int32_t>>;
    record(pfaedle::gtfs::feed& feed): //NOLINT
        feed(feed)
    {
    }

    record(const record& record) = delete;
    record& operator=(const record& record) = delete;

    record(record&& record) noexcept:
        feed(record.feed),
        payload(std::move(record.payload))
    {

    }

    record& operator=(record&& record) noexcept
    {
        this->payload = std::move(record.payload);
        return *this;
    }

    virtual ~record() = default;

protected:
    pfaedle::gtfs::feed& feed;
    payload_type payload;
};
}
