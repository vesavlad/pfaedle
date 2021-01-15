// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_CONFIG_PFAEDLECONFIG_H_
#define PFAEDLE_CONFIG_PFAEDLECONFIG_H_

#include "cppgtfs/gtfs/Route.h"
#include <pfaedle/gtfs/route_type.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace pfaedle::config
{

struct config
{
    std::string dbgOutputPath {"."};
    std::string solveMethod{"global"};
    std::string evalPath{"."};
    std::string shapeTripId;
    std::string outputPath{"gtfs-out"};
    std::string writeOsm;
    std::string osmPath;
    std::string evalDfBins;
    std::vector<std::string> feedPaths;
    std::vector<std::string> configPaths;
    std::set<ad::cppgtfs::gtfs::Route::TYPE> mots;
    std::set<pfaedle::gtfs::route_type> route_type_set;
    bool dropShapes{false};
    bool useHMM{false};
    bool writeGraph{false};
    bool writeCombGraph{false};
    bool evaluate{false};
    bool buildTransitGraph{false};
    bool useCaching{false};
    bool writeOverpass{false};
    bool inPlace{false};
    double gridSize{2000};

    std::string to_string()
    {
        std::stringstream ss;
        ss << "trip-id: " << shapeTripId << "\n"
           << "output-path: " << outputPath << "\n"
           << "write-osm-path: " << writeOsm << "\n"
           << "read-osm-path: " << osmPath << "\n"
           << "debug-output-path: " << dbgOutputPath << "\n"
           << "drop-shapes: " << dropShapes << "\n"
           << "use-hmm: " << useHMM << "\n"
           << "write-graph: " << writeGraph << "\n"
           << "write-cgraph: " << writeCombGraph << "\n"
           << "grid-size: " << gridSize << "\n"
           << "use-cache: " << useCaching << "\n"
           << "write-overpass: " << writeOverpass << "\n"
           << "feed-paths: ";

        for (const auto& p : feedPaths)
        {
            ss << p << " ";
        }

        ss << "\nconfig-paths: ";

        for (const auto& p : configPaths)
        {
            ss << p << " ";
        }

        ss << "\nmots: ";

        for (const auto& mot : mots)
        {
            ss << mot << " ";
        }

        ss << "\n";

        return ss.str();
    }
};

}// namespace pfaedle

#endif// PFAEDLE_CONFIG_PFAEDLECONFIG_H_
