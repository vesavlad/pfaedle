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

#include "pfaedle/config.h"
#include "cppgtfs/Parser.h"
#include "cppgtfs/Writer.h"
#include "configparser/parse_file_exception.h"
#include "pfaedle/config/ConfigReader.h"
#include "pfaedle/config/MotConfig.h"
#include "pfaedle/config/MotConfigReader.h"
#include "pfaedle/eval/Collector.h"
#include "pfaedle/gtfs/Feed.h"
#include "pfaedle/gtfs/Writer.h"
#include "pfaedle/netgraph/Graph.h"
#include "pfaedle/router/ShapeBuilder.h"
#include "pfaedle/trgraph/Graph.h"
#include "pfaedle/trgraph/StatGroup.h"
#include "util/geo/output/GeoGraphJsonOutput.h"
#include "util/geo/output/GeoJsonOutput.h"
#include <logging/logger.h>
#include <logging/scoped_timer.h>
#include "util/Misc.h"

class app
{
public:
    app(int argc, char* argv[]);
    int run();

private:
    pfaedle::config::Config cfg_;
    bool has_config_;
    std::vector<pfaedle::gtfs::Feed> feeds_;
    pfaedle::config::MotConfigReader mot_cfg_reader_;
};


#endif//PFAEDLE_APP_H
