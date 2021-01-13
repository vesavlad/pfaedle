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

class app
{
public:
    app(int argc, char* argv[]);
    int run();

private:
    pfaedle::config::config cfg_;
    bool has_config_;
    std::vector<pfaedle::gtfs::Feed> feeds_;
    pfaedle::config::mot_config_reader mot_cfg_reader_;
};


#endif//PFAEDLE_APP_H
