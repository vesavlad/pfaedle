//
// Created by Vesa Vlad on 02.01.2021.
//

#ifndef PFAEDLE_APP_H
#define PFAEDLE_APP_H

#include <climits>
#include <pwd.h>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <string>
#include <vector>

#include "configparser/parse_file_exception.h"
#include "cppgtfs/Parser.h"
#include "cppgtfs/Writer.h"
#include "pfaedle/config.h"
#include "pfaedle/config/config_reader.h"
#include "pfaedle/config/mot_config.h"
#include "pfaedle/config/mot_config_reader.h"
#include "pfaedle/eval/collector.h"
#include "pfaedle/gtfs/Feed.h"
#include "pfaedle/gtfs/Writer.h"
#include "pfaedle/netgraph/graph.h"
#include "pfaedle/router/shape_builder.h"
#include "pfaedle/trgraph/graph.h"
#include "pfaedle/trgraph/station_group.h"
#include "util/Misc.h"
#include "util/geo/output/GeoGraphJsonOutput.h"
#include "util/geo/output/GeoJsonOutput.h"
#include <logging/logger.h>
#include <logging/scoped_timer.h>

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
    std::vector<pfaedle::gtfs::Feed> feeds_;
    pfaedle::config::mot_config_reader mot_cfg_reader_;
};


#endif//PFAEDLE_APP_H
