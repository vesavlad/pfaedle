#pragma once
namespace pfaedle::gtfs::access
{
enum result_code
{
    OK,
    END_OF_FILE,
    ERROR_INVALID_GTFS_PATH,
    ERROR_FILE_ABSENT,
    ERROR_REQUIRED_FIELD_ABSENT,
    ERROR_INVALID_FIELD_FORMAT
};
}
