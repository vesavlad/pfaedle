#pragma once
#include <map>
#include <string>
#include <variant>

namespace pfaedle::gtfs
{
class feed;

class basic_record
{
public:
    using payload_type = std::map<std::string, std::variant<std::string, double, int32_t>>;

    basic_record() = default;
    virtual ~basic_record() = default;

    basic_record(const basic_record& record) = delete;
    basic_record& operator=(const basic_record& record) = delete;

    basic_record(basic_record&& record) noexcept:
        payload(std::move(record.payload))
    {
    }

    basic_record& operator=(basic_record&& record) noexcept
    {
        this->payload = std::move(record.payload);
        return *this;
    }
private:
    payload_type payload;
};

class record: public basic_record
{
public:
    record(pfaedle::gtfs::feed& feed): //NOLINT
        basic_record(),
        feed(feed)
    {
    }

    record(record&& record) noexcept:
        basic_record(std::move(record)),
        feed(record.feed)
    {
    }

    record& operator=(record&& record) noexcept
    {
        basic_record::operator=(std::move(record));
        return *this;
    }

    ~record() override = default;

protected:
    pfaedle::gtfs::feed& feed;
};

template<typename T>
class gtfs_record: public record
{
public:
};
}
