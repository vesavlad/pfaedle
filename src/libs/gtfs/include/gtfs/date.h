#pragma once
#include <string>
#include <tuple>

namespace pfaedle::gtfs
{
class date
{
public:
    date() = default;
    date(uint16_t year, uint16_t month, uint16_t day);
    explicit date(const std::string & raw_date_str);
    bool is_provided() const;
    std::tuple<uint16_t, uint16_t, uint16_t> get_yyyy_mm_dd() const;
    std::string get_raw_date() const;

private:
    void check_valid() const;

    std::string raw_date;
    uint16_t yyyy = 0;
    uint16_t mm = 0;
    uint16_t dd = 0;
    bool date_is_provided = false;
};
bool operator==(const date & lhs, const date & rhs);
}
