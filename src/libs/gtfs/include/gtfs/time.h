#pragma once
#include <gtfs/misc.h>
#include <tuple>
#include <string>

namespace pfaedle::gtfs
{
// Time in GTFS is in the HH:MM:SS format (H:MM:SS is also accepted)
// Time within a service day can be above 24:00:00, e.g. 28:41:30
class time
{
public:
    time() = default;
    explicit time(const std::string & raw_time_str);
    time(uint16_t hours, uint16_t minutes, uint16_t seconds);
    bool is_provided() const;
    size_t get_total_seconds() const;
    std::tuple<uint16_t, uint16_t, uint16_t> get_hh_mm_ss() const;
    std::string get_raw_time() const;
    bool limit_hours_to_24max();

private:
    void set_total_seconds();
    void set_raw_time();
    bool time_is_provided = false;
    std::string raw_time;
    size_t total_seconds = 0;
    uint16_t hh = 0;
    uint16_t mm = 0;
    uint16_t ss = 0;
};

bool operator==(const time & lhs, const time & rhs);

}

