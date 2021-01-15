#include <gtfs/date.h>
#include <gtfs/misc.h>
#include <gtfs/exceptions.h>

namespace pfaedle::gtfs
{

void date::check_valid() const
{
    if (yyyy < 1000 || yyyy > 9999 || mm < 1 || mm > 12 || dd < 1 || dd > 31)
        throw invalid_field_format("date check failed: out of range. " + std::to_string(yyyy) +
                                 " year, " + std::to_string(mm) + " month, " + std::to_string(dd) +
                                 " day");

    if (mm == 2 && dd > 28)
    {
        // The year is not leap. Days count should be 28.
        if (yyyy % 4 != 0 || (yyyy % 100 == 0 && yyyy % 400 != 0))
            throw invalid_field_format("Invalid days count in February of non-leap year: " +
                                     std::to_string(dd) + " year" + std::to_string(yyyy));

        // The year is leap. Days count should be 29.
        if (dd > 29)
            throw invalid_field_format("Invalid days count in February of leap year: " +
                                     std::to_string(dd) + " year" + std::to_string(yyyy));
    }

    if (dd > 30 && (mm == 4 || mm == 6 || mm == 9 || mm == 11))
        throw invalid_field_format("Invalid days count in month: " + std::to_string(dd) + " days in " +
                                 std::to_string(mm));
}

date::date(uint16_t year, uint16_t month, uint16_t day) : yyyy(year), mm(month), dd(day)
{
    check_valid();
    const std::string mm_str = append_leading_zero(std::to_string(mm));
    const std::string dd_str = append_leading_zero(std::to_string(dd));

    raw_date = std::to_string(yyyy) + mm_str + dd_str;
    date_is_provided = true;
}

date::date(const std::string & raw_date_str) : raw_date(raw_date_str)
{
    if (raw_date.empty())
        return;

    if (raw_date.size() != 8)
        throw invalid_field_format("date is not in YYYY:MM::DD format: " + raw_date_str);

    yyyy = static_cast<uint16_t>(std::stoi(raw_date.substr(0, 4)));
    mm = static_cast<uint16_t>(std::stoi(raw_date.substr(4, 2)));
    dd = static_cast<uint16_t>(std::stoi(raw_date.substr(6, 2)));

    check_valid();

    date_is_provided = true;
}

bool date::is_provided() const { return date_is_provided; }

std::tuple<uint16_t, uint16_t, uint16_t> date::get_yyyy_mm_dd() const
{
    return {yyyy, mm, dd};
}

std::string date::get_raw_date() const { return raw_date; }

bool operator==(const date & lhs, const date & rhs)
{
    return lhs.get_yyyy_mm_dd() == rhs.get_yyyy_mm_dd() && lhs.is_provided() == rhs.is_provided();
}
}
