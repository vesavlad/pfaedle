#pragma once
namespace pfaedle::gtfs
{
enum class stop_location_type
{
    StopOrPlatform = 0,
    Station = 1,
    EntranceExit = 2,
    GenericNode = 3,
    BoardingArea = 4
};
}
