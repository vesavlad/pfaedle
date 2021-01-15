#ifndef PFAEDLE_APP_H
#define PFAEDLE_APP_H

#include <gtfs/feed.h>
#include <pfaedle/config/config.h>
#include <pfaedle/config/mot_config_reader.h>

#include <vector>



enum class ret_code
{
    SUCCESS = 0,
    NO_INPUT_FEED = 1,
    MULT_FEEDS_NOT_ALWD = 2,
    TRIP_NOT_FOUND = 3,
    GTFS_PARSE_ERR = 4,
    NO_OSM_INPUT = 5,
    MOT_CFG_PARSE_ERR = 6,
    OSM_PARSE_ERR = 7,
    GTFS_WRITE_ERR = 8,
    NO_MOT_CFG = 9
};

class app
{
public:
    app(int argc, char* argv[]);
    ret_code run();

private:
    pfaedle::config::config cfg_;
    bool has_config_;
    std::vector<pfaedle::gtfs::feed> feeds_;
    pfaedle::config::mot_config_reader mot_cfg_reader_;
};


#endif//PFAEDLE_APP_H
