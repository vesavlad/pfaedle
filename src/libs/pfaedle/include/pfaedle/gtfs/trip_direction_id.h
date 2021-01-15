#pragma once
namespace pfaedle::gtfs
{
enum class trip_direction_id
{
    DefaultDirection = 0,// e.g. outbound
    OppositeDirection = 1// e.g. inbound
};
}
