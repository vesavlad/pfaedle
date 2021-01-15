#include "app.h"

#include <climits>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <fstream>


#include <pfaedle/config.h>
#include <pfaedle/config/config_reader.h>
#include <pfaedle/config/mot_config.h>
#include <pfaedle/eval/collector.h>
#include <pfaedle/netgraph/graph.h>
#include <pfaedle/router/shape_builder.h>
#include <pfaedle/trgraph/graph.h>
#include <pfaedle/trgraph/station_group.h>

#include <util/Misc.h>
#include <util/geo/output/GeoGraphJsonOutput.h>
#include <util/geo/output/GeoJsonOutput.h>

#include <configparser/parse_file_exception.h>
#include <logging/logger.h>
#include <logging/scoped_timer.h>

#include <gtfs/access/feed_reader.h>
#include <gtfs/access/feed_writter.h>

#ifndef CFG_HOME_SUFFIX
#define CFG_HOME_SUFFIX "/.config"
#endif
#ifndef CFG_DIR
#define CFG_DIR "/etc"
#endif
#ifndef CFG_FILE_NAME
#define CFG_FILE_NAME "pfaedle.cfg"
#endif

namespace
{

std::string get_file_name_mot_str(const pfaedle::router::route_type_set& mots)
{
    std::string mot_str;
    for (const auto& mot : mots)
    {
        mot_str += "-" + pfaedle::gtfs::get_route_type_string(mot);
    }

    return mot_str;
}

std::vector<std::string> get_cfg_paths(const pfaedle::config::config& cfg)
{
    if (!cfg.configPaths.empty())
        return cfg.configPaths;

    std::vector<std::string> ret;


    // install prefix global configuration path, if available
    {
        auto path = pfaedle::install_prefix_path() +
                    std::string(CFG_DIR) + "/" + "pfaedle" + "/" +
                    CFG_FILE_NAME;
        std::ifstream is(path);

        LOG(DEBUG) << "Testing for config file at " << path;
        if (is.good())
        {
            ret.push_back(path);
            LOG(DEBUG) << "Found implicit config file " << path;
        }
    }

    // local user configuration path, if available
    {
        auto path = util::getHomeDir() + CFG_HOME_SUFFIX + "/" + "pfaedle" + "/" + CFG_FILE_NAME;
        std::ifstream is(path);

        LOG(DEBUG) << "Testing for config file at " << path;
        if (is.good())
        {
            ret.push_back(path);
            LOG(DEBUG) << "Found implicit config file " << path;
        }
    }

    // free this here, as we use homedir in the block above

    // CWD
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)))
        {
            auto path = std::string(cwd) + "/" + CFG_FILE_NAME;
            std::ifstream is(path);

            LOG(DEBUG) << "Testing for config file at " << path;
            if (is.good())
            {
                ret.push_back(path);
                LOG(DEBUG) << "Found implicit config file " << path;
            }
        }
    }

    return ret;
}

bool read_config(pfaedle::config::config& cfg, int argc, char** argv)
{
    pfaedle::config::config_reader reader(cfg);
    reader.read(argc, argv);
    return true;
}
}  // namespace


app::app(int argc, char** argv) :
    cfg_{},
    has_config_{read_config(cfg_, argc, argv)},
    feeds_{cfg_.feedPaths.size()},
    mot_cfg_reader_{get_cfg_paths(cfg_)}
{
}

ret_code app::run()
{
    // feed containing the shapes in memory for evaluation
    pfaedle::gtfs::feed eval_feed;


    if (cfg_.osmPath.empty() && !cfg_.writeOverpass)
    {
        LOG_ERROR() << "No OSM input file specified (-x), see --help.";
        return ret_code::NO_OSM_INPUT;
    }

    if (mot_cfg_reader_.get_configs().empty())
    {
        LOG(ERROR) << "No MOT configurations specified and no implicit "
                      "configurations found, see --help.";
        return ret_code::NO_MOT_CFG;
    }

    if (cfg_.feedPaths.size() == 1)
    {
        if (cfg_.inPlace)
        {
            cfg_.outputPath = cfg_.feedPaths.front();
        }
        if (!cfg_.writeOverpass)
        {
            LOG(INFO) << "Reading " << cfg_.feedPaths.front() << " ...";
        }

        pfaedle::gtfs::access::feed_reader reader(feeds_.front(), cfg_.feedPaths.front());
        pfaedle::gtfs::access::feed_reader::read_config read_config;
        read_config.shapes = cfg_.evaluate;
        if(auto res = reader.read(read_config); res != pfaedle::gtfs::access::result_code::OK)
        {
            LOG(ERROR) << "Could not parse input GTFS feed, reason was:";
            LOG(ERROR) << res.message;
            return ret_code::GTFS_PARSE_ERR;
        }

        if (!cfg_.writeOverpass)
            LOG(INFO) << "Done.";
    }
    else if (!cfg_.writeOsm.empty() || cfg_.writeOverpass)
    {
        for (size_t i = 0; i < cfg_.feedPaths.size(); i++)
        {
            if (!cfg_.writeOverpass)
            {
                LOG(INFO) << "Reading " << cfg_.feedPaths[i] << " ...";
            }

            pfaedle::gtfs::access::feed_reader reader(feeds_[i], cfg_.feedPaths[i]);
            pfaedle::gtfs::access::feed_reader::read_config read_config;
            read_config.shapes = cfg_.evaluate;
            if(auto res = reader.read(read_config); res != pfaedle::gtfs::access::result_code::OK)
            {
                LOG(ERROR) << "Could not parse input GTFS feed, reason was:";
                LOG(ERROR) << res.message;
                return ret_code::GTFS_PARSE_ERR;
            }
            if (!cfg_.writeOverpass)
            {
                LOG(INFO) << "Done.";
            }
        }
    }
    else if (cfg_.feedPaths.size() > 1)
    {
        std::cerr << "Multiple feeds only allowed in filter mode." << std::endl;
        return ret_code::MULT_FEEDS_NOT_ALWD;
    }

    LOG(DEBUG) << "Read " << mot_cfg_reader_.get_configs().size() << " unique MOT configs.";
    pfaedle::router::route_type_set cmd_route_types = cfg_.route_type_set;
    pfaedle::gtfs::trip* single_trip = nullptr;

    if (!cfg_.shapeTripId.empty())
    {
        if (cfg_.feedPaths.empty())
        {
            std::cout << "No input feed specified, see --help" << std::endl;
            return ret_code::NO_INPUT_FEED;
        }
        if (!feeds_.front().trips.count(cfg_.shapeTripId))
        {
            LOG(ERROR) << "Trip #" << cfg_.shapeTripId << " not found.";
            return ret_code::TRIP_NOT_FOUND;
        }
        else
        {
            single_trip = &feeds_.front().trips[cfg_.shapeTripId];
        }
    }

    if (!cfg_.writeOsm.empty())
    {
        LOG(INFO) << "Writing filtered XML to " << cfg_.writeOsm << " ...";
        pfaedle::osm::bounding_box box(BOX_PADDING);
        for (size_t i = 0; i < cfg_.feedPaths.size(); i++)
        {
            pfaedle::router::shape_builder::get_gtfs_box(feeds_[i], cmd_route_types, cfg_.shapeTripId, true, box);
        }

        pfaedle::osm::osm_builder osm_builder;
        std::vector<pfaedle::osm::osm_read_options> opts;
        for (const auto& o : mot_cfg_reader_.get_configs())
        {
            if (std::find_first_of(o.route_types.begin(),
                                   o.route_types.end(),
                                   cmd_route_types.begin(),
                                   cmd_route_types.end()) != o.route_types.end())
            {
                opts.push_back(o.osmBuildOpts);
            }
        }
        osm_builder.filter_write(cfg_.osmPath, cfg_.writeOsm, opts, box);
        return ret_code::SUCCESS;
    }
    else if (cfg_.writeOverpass)
    {
        pfaedle::osm::bounding_box box(BOX_PADDING);
        for (size_t i = 0; i < cfg_.feedPaths.size(); i++)
        {
            pfaedle::router::shape_builder::get_gtfs_box(feeds_[i], cmd_route_types, cfg_.shapeTripId, true, box);
        }

        pfaedle::osm::osm_builder osm_builder;
        std::vector<pfaedle::osm::osm_read_options> opts;
        for (const auto& o : mot_cfg_reader_.get_configs())
        {
            if (std::find_first_of(o.route_types.begin(), o.route_types.end(), cmd_route_types.begin(),
                                   cmd_route_types.end()) != o.route_types.end())
            {
                opts.push_back(o.osmBuildOpts);
            }
        }
        osm_builder.overpass_query_write(std::cout, opts, box);
        return ret_code::SUCCESS;
    }
    else if (cfg_.feedPaths.empty())
    {
        std::cout << "No input feed specified, see --help" << std::endl;
        return ret_code::NO_INPUT_FEED;
    }

    std::vector<double> df_bins;
    auto df_bin_strings = util::split(std::string(cfg_.evalDfBins), ',');
    df_bins.reserve(df_bin_strings.size());

    for (const auto& st : df_bin_strings)
        df_bins.push_back(atof(st.c_str()));

    pfaedle::eval::collector collector(cfg_.evalPath, df_bins);

    for (const auto& mot_cfg : mot_cfg_reader_.get_configs())
    {
        std::string file_post;
        auto used_mots = pfaedle::router::route_type_section(mot_cfg.route_types, cmd_route_types);

        if (used_mots.empty())
            continue;

        if (single_trip && single_trip->route.has_value() && !used_mots.count(single_trip->route.value().get().route_type))
            continue;

        if (mot_cfg_reader_.get_configs().size() > 1)
            file_post = get_file_name_mot_str(used_mots);

        const std::string mot_str = pfaedle::router::get_mot_str(used_mots);
        LOG(INFO) << "Calculating shapes for mots " << mot_str;

        pfaedle::router::feed_stops f_stops =
                pfaedle::router::write_mot_stops(feeds_.front(), used_mots, cfg_.shapeTripId);

        pfaedle::osm::restrictor restrictor;
        pfaedle::trgraph::graph graph;
        pfaedle::osm::osm_builder osm_builder;

        pfaedle::osm::bounding_box box(BOX_PADDING);
        pfaedle::router::shape_builder::get_gtfs_box(feeds_.front(), cmd_route_types, cfg_.shapeTripId, cfg_.dropShapes, box);

        if (!f_stops.empty())
        {
            osm_builder.read(cfg_.osmPath,
                             mot_cfg.osmBuildOpts,
                             graph,
                             box,
                             cfg_.gridSize,
                             f_stops,
                             restrictor);
        }

        // TODO(patrick): move this somewhere else
        for (auto& feed_stop : f_stops)
        {
            if (feed_stop.second)
            {
                feed_stop.second->pl().get_si()->get_group()->write_penalties(
                        mot_cfg.osmBuildOpts.trackNormzer,
                        mot_cfg.routingOpts.platformUnmatchedPen,
                        mot_cfg.routingOpts.stationDistPenFactor,
                        mot_cfg.routingOpts.nonOsmPen);
            }
        }

        pfaedle::router::shape_builder shape_builder(feeds_.front(),
                                                    eval_feed,
                                                    cmd_route_types,
                                                    mot_cfg,
                                                     collector,
                                                    graph,
                                                    f_stops,
                                                     restrictor,
                                                    cfg_);

        if (cfg_.writeGraph)
        {
            LOG(INFO) << "Outputting graph.json...";
            util::geo::output::GeoGraphJsonOutput out;
            mkdir(cfg_.dbgOutputPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            std::ofstream fstr(cfg_.dbgOutputPath + "/graph.json");
            out.printLatLng(shape_builder.get_graph(), fstr);
            fstr.close();
        }

        if (single_trip)
        {
            LOG(INFO) << "Outputting path.json...";
            mkdir(cfg_.dbgOutputPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            std::ofstream pstr(cfg_.dbgOutputPath + "/path.json");
            util::geo::output::GeoJsonOutput o(pstr);

            auto l = shape_builder.get_shape_line(*single_trip);

            // reproject to WGS84 to match RFC 7946
            o.printLatLng(l, {});

            o.flush();
            pstr.close();

            return ret_code::SUCCESS;
        }

        pfaedle::netgraph::graph ng;
        shape_builder.get_shape(ng);

        if (cfg_.buildTransitGraph)
        {
            util::geo::output::GeoGraphJsonOutput out;
            LOG(INFO) << "Outputting trgraph" + file_post + ".json...";
            mkdir(cfg_.dbgOutputPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            std::ofstream fstr(cfg_.dbgOutputPath + "/trgraph" + file_post + ".json");
            out.printLatLng(ng, fstr);
            fstr.close();
        }
    }

    if (cfg_.evaluate)
        collector.print_stats(std::cout);

    if (!cfg_.feedPaths.empty())
    {
        mkdir(cfg_.outputPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        LOG(INFO) << "Writing output GTFS to " << cfg_.outputPath << " ...";
        pfaedle::gtfs::access::feed_writter writter(feeds_[0], cfg_.outputPath);
        pfaedle::gtfs::access::feed_writter::write_config config;
        if(auto res = writter.write(config); res != pfaedle::gtfs::access::result_code::OK)
        {
            LOG(ERROR) << "Could not write final GTFS feed, reason was:";
            LOG(ERROR) << res.message;
            return ret_code::GTFS_WRITE_ERR;
        }
    }
    return ret_code::SUCCESS;
}
