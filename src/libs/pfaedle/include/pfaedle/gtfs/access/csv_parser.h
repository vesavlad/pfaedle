#pragma once
#include <pfaedle/gtfs/access/result.h>
#include <pfaedle/gtfs/misc.h>

#include <fstream>
#include <vector>
#include <string>
#include <map>

namespace pfaedle::gtfs::access
{
class csv_parser
{
public:
    csv_parser() = default;
    explicit csv_parser(const std::string & gtfs_directory);

    result read_header(const std::string & csv_filename);
    result read_row(std::map<std::string, std::string> & obj);

    static std::vector<std::string> split_record(const std::string & record,
                                                        bool is_header = false);

private:
    std::vector<std::string> field_sequence;
    std::string gtfs_path;
    std::ifstream csv_stream;
};


}
