#include "app.h"
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

std::string get_file_name_mot_str(const pfaedle::router::MOTs& mots)
{
    std::string mot_str;
    for (const auto& mot : mots)
    {
        mot_str += "-" + ad::cppgtfs::gtfs::flat::Route::getTypeString(mot);
    }

    return mot_str;
}

std::vector<std::string> get_cfg_paths(const pfaedle::config::Config& cfg)
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

bool read_config(pfaedle::config::Config& cfg, int argc, char** argv)
{
    pfaedle::config::ConfigReader reader(cfg);
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

int app::run()
{
    // feed containing the shapes in memory for evaluation
    ad::cppgtfs::gtfs::Feed eval_feed;

    if (cfg_.osmPath.empty() && !cfg_.writeOverpass)
    {
        std::cerr << "No OSM input file specified (-x), see --help." << std::endl;
        exit(static_cast<int>(ret_code::NO_OSM_INPUT));
    }

    if (mot_cfg_reader_.getConfigs().empty())
    {
        LOG(ERROR) << "No MOT configurations specified and no implicit "
                      "configurations found, see --help.";
        exit(static_cast<int>(ret_code::NO_MOT_CFG));
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

        try
        {
            ad::cppgtfs::Parser p;
            p.parse(&feeds_.front(), cfg_.feedPaths.front());
            if (cfg_.evaluate)
            {
                // read the shapes and store them in memory
                p.parseShapes(&eval_feed, cfg_.feedPaths.front());
            }
        }
        catch (const ad::cppgtfs::ParserException& ex)
        {
            LOG(ERROR) << "Could not parse input GTFS feed, reason was:";
            LOG(ERROR) << ex.what();
            exit(static_cast<int>(ret_code::GTFS_PARSE_ERR));
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

            ad::cppgtfs::Parser p;
            try
            {
                p.parse(&feeds_[i], cfg_.feedPaths[i]);
            }
            catch (const ad::cppgtfs::ParserException& ex)
            {
                LOG(ERROR) << "Could not parse input GTFS feed, reason was:";
                std::cerr << ex.what() << std::endl;
                exit(static_cast<int>(ret_code::GTFS_PARSE_ERR));
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
        exit(static_cast<int>(ret_code::MULT_FEEDS_NOT_ALWD));
    }

    LOG(DEBUG) << "Read " << mot_cfg_reader_.getConfigs().size() << " unique MOT configs.";
    pfaedle::router::MOTs cmd_cfg_mots = cfg_.mots;
    pfaedle::gtfs::Trip* single_trip = nullptr;

    if (!cfg_.shapeTripId.empty())
    {
        if (cfg_.feedPaths.empty())
        {
            std::cout << "No input feed specified, see --help" << std::endl;
            exit(static_cast<int>(ret_code::NO_INPUT_FEED));
        }
        single_trip = feeds_.front().getTrips().get(cfg_.shapeTripId);
        if (!single_trip)
        {
            LOG(ERROR) << "Trip #" << cfg_.shapeTripId << " not found.";
            exit(static_cast<int>(ret_code::TRIP_NOT_FOUND));
        }
    }

    if (!cfg_.writeOsm.empty())
    {
        LOG(INFO) << "Writing filtered XML to " << cfg_.writeOsm << " ...";
        pfaedle::osm::BBoxIdx box(BOX_PADDING);
        for (size_t i = 0; i < cfg_.feedPaths.size(); i++)
        {
            pfaedle::router::ShapeBuilder::getGtfsBox(feeds_[i], cmd_cfg_mots, cfg_.shapeTripId, true, box);
        }
        pfaedle::osm::OsmBuilder osm_builder;
        std::vector<pfaedle::osm::OsmReadOpts> opts;
        for (const auto& o : mot_cfg_reader_.getConfigs())
        {
            if (std::find_first_of(o.mots.begin(),
                                   o.mots.end(),
                                   cmd_cfg_mots.begin(),
                                   cmd_cfg_mots.end()) != o.mots.end())
            {
                opts.push_back(o.osmBuildOpts);
            }
        }
        osm_builder.filterWrite(cfg_.osmPath, cfg_.writeOsm, opts, box);
        exit(static_cast<int>(ret_code::SUCCESS));
    }
    else if (cfg_.writeOverpass)
    {
        pfaedle::osm::BBoxIdx box(BOX_PADDING);
        for (size_t i = 0; i < cfg_.feedPaths.size(); i++)
        {
            pfaedle::router::ShapeBuilder::getGtfsBox(feeds_[i], cmd_cfg_mots, cfg_.shapeTripId, true, box);
        }
        pfaedle::osm::OsmBuilder osm_builder;
        std::vector<pfaedle::osm::OsmReadOpts> opts;
        for (const auto& o : mot_cfg_reader_.getConfigs())
        {
            if (std::find_first_of(o.mots.begin(), o.mots.end(), cmd_cfg_mots.begin(), cmd_cfg_mots.end()) != o.mots.end())
            {
                opts.push_back(o.osmBuildOpts);
            }
        }
        osm_builder.overpassQryWrite(std::cout, opts, box);
        exit(static_cast<int>(ret_code::SUCCESS));
    }
    else if (cfg_.feedPaths.empty())
    {
        std::cout << "No input feed specified, see --help" << std::endl;
        exit(static_cast<int>(ret_code::NO_INPUT_FEED));
    }

    std::vector<double> df_bins;
    auto df_bin_strings = util::split(std::string(cfg_.evalDfBins), ',');
    df_bins.reserve(df_bin_strings.size());

    for (const auto& st : df_bin_strings)
        df_bins.push_back(atof(st.c_str()));
    pfaedle::eval::Collector ecoll(cfg_.evalPath, df_bins);

    for (const auto& mot_cfg : mot_cfg_reader_.getConfigs())
    {
        std::string file_post;
        auto used_mots = pfaedle::router::motISect(mot_cfg.mots, cmd_cfg_mots);

        if (used_mots.empty())
            continue;

        if (single_trip && !used_mots.count(single_trip->getRoute()->getType()))
            continue;

        if (mot_cfg_reader_.getConfigs().size() > 1)
            file_post = get_file_name_mot_str(used_mots);

        const std::string mot_str = pfaedle::router::getMotStr(used_mots);
        LOG(INFO) << "Calculating shapes for mots " << mot_str;

        pfaedle::router::FeedStops f_stops =
                pfaedle::router::writeMotStops(feeds_.front(), used_mots, cfg_.shapeTripId);

        pfaedle::osm::Restrictor restr;
        pfaedle::trgraph::Graph graph;
        pfaedle::osm::OsmBuilder osm_builder;

        pfaedle::osm::BBoxIdx box(BOX_PADDING);
        pfaedle::router::ShapeBuilder::getGtfsBox(feeds_.front(), cmd_cfg_mots, cfg_.shapeTripId, cfg_.dropShapes, box);

        if (!f_stops.empty())
        {
            osm_builder.read(cfg_.osmPath,
                             mot_cfg.osmBuildOpts,
                             graph,
                             box,
                             cfg_.gridSize,
                             f_stops,
                             restr);
        }

        // TODO(patrick): move this somewhere else
        for (auto& feed_stop : f_stops)
        {
            if (feed_stop.second)
            {
                feed_stop.second->pl().getSI()->getGroup()->writePens(
                        mot_cfg.osmBuildOpts.trackNormzer,
                        mot_cfg.routingOpts.platformUnmatchedPen,
                        mot_cfg.routingOpts.stationDistPenFactor,
                        mot_cfg.routingOpts.nonOsmPen);
            }
        }

        pfaedle::router::ShapeBuilder shape_builder(feeds_.front(),
                                                    eval_feed,
                                                    cmd_cfg_mots,
                                                    mot_cfg,
                                                    ecoll,
                                                    graph,
                                                    f_stops,
                                                    restr,
                                                    cfg_);

        if (cfg_.writeGraph)
        {
            LOG(INFO) << "Outputting graph.json...";
            util::geo::output::GeoGraphJsonOutput out;
            mkdir(cfg_.dbgOutputPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            std::ofstream fstr(cfg_.dbgOutputPath + "/graph.json");
            out.printLatLng(shape_builder.getGraph(), fstr);
            fstr.close();
        }

        if (single_trip)
        {
            LOG(INFO) << "Outputting path.json...";
            mkdir(cfg_.dbgOutputPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            std::ofstream pstr(cfg_.dbgOutputPath + "/path.json");
            util::geo::output::GeoJsonOutput o(pstr);

            auto l = shape_builder.shapeL(*single_trip);

            // reproject to WGS84 to match RFC 7946
            o.printLatLng(l, {});

            o.flush();
            pstr.close();

            exit(static_cast<int>(ret_code::SUCCESS));
        }

        pfaedle::netgraph::Graph ng;
        shape_builder.shape(ng);

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
        ecoll.printStats(&std::cout);

    if (!cfg_.feedPaths.empty())
    {
        try
        {
            mkdir(cfg_.outputPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            LOG(INFO) << "Writing output GTFS to " << cfg_.outputPath << " ...";
            pfaedle::gtfs::Writer w;
            w.write(&feeds_[0], cfg_.outputPath);
        }
        catch (const ad::cppgtfs::WriterException& ex)
        {
            LOG(ERROR) << "Could not write final GTFS feed, reason was:";
            std::cerr << ex.what() << std::endl;
            exit(static_cast<int>(ret_code::GTFS_WRITE_ERR));
        }
    }
    return static_cast<int>(ret_code::SUCCESS);
}
