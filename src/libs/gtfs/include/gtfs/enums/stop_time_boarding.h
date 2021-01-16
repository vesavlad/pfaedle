#pragma once
namespace pfaedle::gtfs
{
enum class stop_time_boarding
{
    RegularlyScheduled = 0,
    No = 1,                 // Not available
    Phone = 2,              // Must phone agency to arrange
    CoordinateWithDriver = 3// Must coordinate with driver to arrange
};
}
