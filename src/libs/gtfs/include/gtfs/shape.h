#pragma once
#include <gtfs/types.h>
#include <vector>

namespace pfaedle::gtfs
{
struct shape_point
{
    // Required:
    Id shape_id;
    double shape_pt_lat = 0.0;
    double shape_pt_lon = 0.0;
    size_t shape_pt_sequence = 0;

    // Optional:
    double shape_dist_traveled = 0;
};

struct shape
{
    Id shape_id;
    std::vector<shape_point> points;

    decltype(points.empty()) empty() const
    {
        return points.empty();
    }

    decltype(points.size()) size() const
    {
        return points.size();
    }
};
}
