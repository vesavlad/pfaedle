#include <pfaedle/gtfs/time.h>
#include <pfaedle/gtfs/exceptions.h>

namespace pfaedle::gtfs
{

bool time::limit_hours_to_24max()
{
    if (hh < 24)
        return false;

    hh = hh % 24;
    set_total_seconds();
    set_raw_time();
    return true;
}

void time::set_total_seconds() { total_seconds = hh * 60 * 60 + mm * 60 + ss; }

void time::set_raw_time()
{
    const std::string hh_str = append_leading_zero(std::to_string(hh), false);
    const std::string mm_str = append_leading_zero(std::to_string(mm));
    const std::string ss_str = append_leading_zero(std::to_string(ss));

    raw_time = hh_str + ":" + mm_str + ":" + ss_str;
}

// Time in the HH:MM:SS format (H:MM:SS is also accepted). Used as type for Time GTFS fields.
time::time(const std::string & raw_time_str) : raw_time(raw_time_str)
{
    if (raw_time_str.empty())
        return;

    const size_t len = raw_time.size();
    if (!(len == 7 || len == 8) || (raw_time[len - 3] != ':' && raw_time[len - 6] != ':'))
        throw invalid_field_format("time is not in [H]H:MM:SS format: " + raw_time_str);

    hh = static_cast<uint16_t>(std::stoi(raw_time.substr(0, len - 6)));
    mm = static_cast<uint16_t>(std::stoi(raw_time.substr(len - 5, 2)));
    ss = static_cast<uint16_t>(std::stoi(raw_time.substr(len - 2)));

    if (mm > 60 || ss > 60)
        throw invalid_field_format("time minutes/seconds wrong value: " + std::to_string(mm) +
                                 " minutes, " + std::to_string(ss) + " seconds");

    set_total_seconds();
    time_is_provided = true;
}

time::time(uint16_t hours, uint16_t minutes, uint16_t seconds)
        : hh(hours), mm(minutes), ss(seconds)
{
    if (mm > 60 || ss > 60)
        throw invalid_field_format("time is out of range: " + std::to_string(mm) + "minutes " +
                                 std::to_string(ss) + "seconds");

    set_total_seconds();
    set_raw_time();
    time_is_provided = true;
}

bool time::is_provided() const { return time_is_provided; }

size_t time::get_total_seconds() const { return total_seconds; }

std::tuple<uint16_t, uint16_t, uint16_t> time::get_hh_mm_ss() const { return {hh, mm, ss}; }

std::string time::get_raw_time() const { return raw_time; }

bool operator==(const time& lhs, const time& rhs)
{
    return lhs.get_hh_mm_ss() == rhs.get_hh_mm_ss() && lhs.is_provided() == rhs.is_provided();
}
}
