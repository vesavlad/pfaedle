#pragma once
namespace pfaedle::gtfs
{
enum class fare_transfers
{
    No = 0,  // No transfers permitted on this fare
    Once = 1,
    Twice = 2,
    Unlimited = 3
};
}
