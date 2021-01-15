#pragma once
namespace pfaedle::gtfs
{
enum class transfer_type
{
    Recommended = 0,
    Timed = 1,
    MinimumTime = 2,
    NotPossible = 3
};
}
