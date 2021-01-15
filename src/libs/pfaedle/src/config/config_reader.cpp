// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/config/config_reader.h"
#include "pfaedle/config.h"
#include "pfaedle/definitions.h"
#include <pfaedle/gtfs/misc.h>
#include <pfaedle/gtfs/route_type.h>

#include "util/String.h"

#include <logging/logger.h>
#include <exception>
#include <getopt.h>
#include <iostream>
#include <string>

namespace pfaedle::config
{
static const char* YEAR = static_cast<const char*>(__DATE__) + 7;
static const char* COPY = "University of Freiburg - Chair of Algorithms and Data Structures";
static const char* AUTHORS = "Patrick Brosi <brosi@informatik.uni-freiburg.de> \n Vlad Vesa <vlad.vesa@outlook.com>";

// _____________________________________________________________________________
void config_reader::help(const char* bin)
{
    std::cout << std::setfill(' ') << std::left << "pfaedle GTFS map matcher "
              << pfaedle::long_version() << "\n(built " << __DATE__ << " " << __TIME__
              << " with geometry precision <" << PFAEDLE_PRECISION_STR << ">)\n\n"
              << "(C) " << YEAR << " " << COPY << "\n"
              << "Authors: " << AUTHORS << "\n\n"
              << "Usage: " << bin
              << " -x <OSM FILE> <GTFS FEED>\n\n"
              << "Allowed options:\n\n"
              << "General:\n"
              << std::setw(35) << "  -v [ --version ]"
              << "print version\n"
              << std::setw(35) << "  -h [ --help ]"
              << "show this help message\n"
              << std::setw(35) << "  -D [ --drop-shapes ]"
              << "drop shapes already present in the feed and\n"
              << std::setw(35) << " "
              << "  recalculate them\n"
              << "\nInput:\n"
              << std::setw(35) << "  -c [ --config ] arg"
              << "pfaedle config file\n"
              << std::setw(35) << "  -i [ --input ] arg"
              << "gtfs feed(s), may also be given as positional\n"
              << std::setw(35) << " "
              << "  parameter (see usage)\n"
              << std::setw(35) << "  -x [ --osm-file ] arg"
              << "OSM xml input file\n"
              << std::setw(35) << "  -m [ --mots ] arg (=all)"
              << "MOTs to calculate shapes for, comma sep.,\n"
              << std::setw(35) << " "
              << "  either as string "
                 "{all, tram | streetcar,\n"
              << std::setw(35) << " "
              << "  subway | metro, rail | train, bus,\n"
              << std::setw(35) << " "
              << "  ferry | boat | ship, cablecar, gondola,\n"
              << std::setw(35) << " "
              << "  funicular, coach} or as GTFS mot codes\n"
              << "\nOutput:\n"
              << std::setw(35) << "  -o [ --output ] arg (=gtfs-out)"
              << "GTFS output path\n"
              << std::setw(35) << "  -X [ --osm-out ] arg"
              << "if specified, a filtered OSM file will be\n"
              << std::setw(35) << " "
              << "  written to <arg>\n"
              << std::setw(35) << "  --inplace"
              << "overwrite input GTFS feed with output feed\n"
              << "\nDebug Output:\n"
              << std::setw(35) << "  -d [ --dbg-path ] arg (=.)"
              << "output path for debug files\n"
              << std::setw(35) << "  --write-trgraph"
              << "write transit graph as GeoJSON to\n"
              << std::setw(35) << " "
              << "  <dbg-path>/trgraph.json\n"
              << std::setw(35) << "  --write-graph"
              << "write routing graph as GeoJSON to\n"
              << std::setw(35) << " "
              << "  <dbg-path>/graph.json\n"
              << std::setw(35) << "  --write-cgraph"
              << "if -T is set, write combination graph as\n"
              << std::setw(35) << " "
              << "  GeoJSON to "
                 "<dbg-path>/combgraph.json\n"
              << std::setw(35) << "  --method arg (=global)"
              << "matching method to use, either 'global'\n"
              << std::setw(35) << " "
              << "  (based on HMM), 'greedy' or "
                 "'greedy2'\n"
              << std::setw(35) << "  --eval"
              << "evaluate existing shapes against matched\n"
              << std::setw(35) << " "
              << "  shapes and print results\n"
              << std::setw(35) << "  --eval-path arg (=.)"
              << "path for eval file output\n"
              << std::setw(35) << "  --eval-df-bins arg (= )"
              << "bins to use for d_f histogram, comma sep.\n"
              << std::setw(35) << " "
              << "  (e.g. 10,20,30,40)\n"
              << "\nMisc:\n"
              << std::setw(35) << "  -T [ --trip-id ] arg"
              << "Do routing only for trip <arg>, write result \n"
              << std::setw(35) << " "
              << "  to <dbg-path>/path.json\n"
              << std::setw(35) << "  --overpass"
              << "Output overpass query for matching OSM data\n"
              << std::setw(35) << "  --grid-size arg (=2000)"
              << "Grid cell size\n"
              << std::setw(35) << "  --use-route-cache"
              << "(experimental) cache intermediate routing\n"
              << std::setw(35) << " "
              << "  results\n";
}
config_reader::config_reader(config& cfg) :
    config_{cfg}
{
//    cxxopts::Options options("PFAEDLE", "One line description of MyProgram");
//    options.add_options()
//            ("b,bar", "Param bar", cxxopts::value<std::string>())
//            ("d,debug", "Enable debugging", cxxopts::value<bool>()->default_value("false"))
//            ("f,foo", "Param foo", cxxopts::value<int>()->default_value("10"))
//            ("h,help", "Print usage")
//            ;
}

// _____________________________________________________________________________
void config_reader::read(int argc, char** argv)
{
    std::string mot_str = "all";
    bool print_opts = false;

    struct option ops[] = {{"output", required_argument, nullptr, 'o'},
                           {"input", required_argument, nullptr, 'i'},
                           {"config", required_argument, nullptr, 'c'},
                           {"osm-file", required_argument, nullptr, 'x'},
                           {"drop-shapes", required_argument, nullptr, 'D'},
                           {"mots", required_argument, nullptr, 'm'},
                           {"grid-size", required_argument, nullptr, 'g'},
                           {"overpass", no_argument, nullptr, 'a'},
                           {"osm-out", required_argument, nullptr, 'X'},
                           {"trip-id", required_argument, nullptr, 'T'},
                           {"write-graph", no_argument, nullptr, 1},
                           {"write-cgraph", no_argument, nullptr, 2},
                           {"write-trgraph", no_argument, nullptr, 4},
                           {"method", required_argument, nullptr, 5},
                           {"eval", no_argument, nullptr, 3},
                           {"eval-path", required_argument, nullptr, 6},
                           {"eval-df-bins", required_argument, nullptr, 7},
                           {"dbg-path", required_argument, nullptr, 'd'},
                           {"version", no_argument, nullptr, 'v'},
                           {"help", no_argument, nullptr, 'h'},
                           {"inplace", no_argument, nullptr, 9},
                           {"use-route-cache", no_argument, nullptr, 8},
                           {nullptr, 0, nullptr, 0}};

    char c = 0;
    while ((c = getopt_long(argc, argv, ":o:hvi:c:x:Dm:g:X:T:d:p", ops, nullptr)) != -1)
    {
        switch (c)
        {
            case 1:
                config_.writeGraph = true;
                break;
            case 2:
                config_.writeCombGraph = true;
                break;
            case 3:
                config_.evaluate = true;
                break;
            case 4:
                config_.buildTransitGraph = true;
                break;
            case 5:
                config_.solveMethod = optarg;
                break;
            case 6:
                config_.evalPath = optarg;
                break;
            case 7:
                config_.evalDfBins = optarg;
                break;
            case 8:
                config_.useCaching = true;
                break;
            case 'o':
                config_.outputPath = optarg;
                break;
            case 'i':
                config_.feedPaths.emplace_back(optarg);
                break;
            case 'c':
                config_.configPaths.emplace_back(optarg);
                break;
            case 'x':
                config_.osmPath = optarg;
                break;
            case 'D':
                config_.dropShapes = true;
                break;
            case 'm':
                mot_str = optarg;
                break;
            case 'g':
                config_.gridSize = atof(optarg);
                break;
            case 'X':
                config_.writeOsm = optarg;
                break;
            case 'T':
                config_.shapeTripId = optarg;
                break;
            case 'd':
                config_.dbgOutputPath = optarg;
                break;
            case 'a':
                config_.writeOverpass = true;
                break;
            case 9:
                config_.inPlace = true;
                break;
            case 'v':
                std::cout << "pfaedle " << pfaedle::short_version() << " (built " << __DATE__ << " "
                          << __TIME__ << " with geometry precision <"
                          << PFAEDLE_PRECISION_STR << ">)\n"
                          << "(C) " << YEAR << " " << COPY << "\n"
                          << "Authors: " << AUTHORS << "\nGNU General Public "
                                                       "License v3.0\n";
                exit(0);
            case 'p':
                print_opts = true;
                break;
            case 'h':
                help(argv[0]);
                exit(0);
            case ':':
                std::cerr << argv[optind - 1];
                std::cerr << " requires an argument" << std::endl;
                exit(1);
            case '?':
                std::cerr << argv[optind - 1];
                std::cerr << " option unknown" << std::endl;
                exit(1);
                break;
            default:
                std::cerr << "Error while parsing arguments" << std::endl;
                exit(1);
                break;
        }
    }

    for (int i = optind; i < argc; i++)
        config_.feedPaths.emplace_back(argv[i]);

    auto v = util::split(mot_str, ',');
    for (const auto& mot_str : v)
    {
        const auto& mots = ad::cppgtfs::gtfs::flat::Route::getTypesFromString(util::trim(mot_str));
        config_.mots.insert(mots.begin(), mots.end());

        const auto& mots_alternative = pfaedle::gtfs::get_route_types_from_string(util::trim(mot_str));
        config_.route_type_set.insert(mots_alternative.begin(), mots_alternative.end());
    }

    if (print_opts)
    {
        LOG_INFO() << "\nConfigured options:\n\n"
                   << config_.to_string();
    }
}
}
