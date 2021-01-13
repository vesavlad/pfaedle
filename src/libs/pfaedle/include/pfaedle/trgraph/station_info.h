// Copyright 2017, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_STATINFO_H_
#define PFAEDLE_TRGRAPH_STATINFO_H_

#include <string>
#include <vector>
#include <unordered_map>

namespace pfaedle::trgraph
{

// forward declaration
class station_group;

/*
 * Meta information (name, alternative names, track, group...) of a single stop
 */
class station_info
{
public:
    station_info();
    station_info(const station_info& si);
    station_info(const std::string& name, const std::string& track, bool _fromOsm);
    ~station_info();

    // Return this stops names.
    const std::string& get_name() const;

    // Return this stops track or empty string, if none.
    const std::string& get_track() const;

    // Add an alternative name for this station.
    void add_alternative_name(const std::string& name);

    // Return all alternative names for this station.
    const std::vector<std::string>& get_alternative_names() const;

    // Set the track of this stop.
    void set_track(const std::string& tr);

    // Return the similarity between this stop and other
    double simi(const station_info* other) const;

    // Set this stations group.
    void set_group(station_group* g);

    // Return this stations group.
    station_group* get_group() const;

    // True if this stop was from osm
    bool is_from_osm() const;

    // Set this stop as coming from osm
    void set_is_from_osm(bool is);

#ifdef PFAEDLE_STATION_IDS
    const std::string& get_id() const
    {
        return _id;
    }
    void set_id(const std::string& id) { _id = id; }
#endif

private:
    std::string _name;
    std::vector<std::string> _altNames;
    std::string _track;
    bool _fromOsm;
    station_group* _group;

#ifdef PFAEDLE_STATION_IDS
    // debug feature to store station ids from both OSM
    // and GTFS
    std::string _id;
#endif

    static std::unordered_map<const station_group*, size_t> _groups;
    static void unRefGroup(station_group* g);
};
}  // namespace pfaedle::trgraph

#endif  // PFAEDLE_TRGRAPH_STATINFO_H_
