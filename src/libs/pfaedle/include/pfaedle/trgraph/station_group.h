// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_STATGROUP_H_
#define PFAEDLE_TRGRAPH_STATGROUP_H_

#include <gtfs/stop.h>
#include "pfaedle/router/router.h"
#include "pfaedle/trgraph/graph.h"
#include "pfaedle/trgraph/normalizer.h"
#include <set>
#include <string>
#include <unordered_map>

namespace pfaedle::trgraph
{

/*
 * A group of stations that belong together semantically (for example, multiple
 * stop points of a larger bus station)
 */
class station_group
{
public:
    station_group();
    station_group(const station_group& a) = delete;

    // Add a stop s to this station group
    void add_stop(const gtfs::stop* s);

    // Add a node n to this station group
    void add_node(trgraph::node* n);

    // Return all nodes contained in this group
    const std::set<trgraph::node*>& get_nodes() const;
    std::set<trgraph::node*>& get_nodes();

    // Return all stops contained in this group
    const std::set<const gtfs::stop*>& get_stops() const;

    // Remove a node from this group
    void remove_node(trgraph::node* n);

    // All nodes in other will be in this group, their SI's updated, and the
    // "other" group deleted.
    void merge(station_group* other);

    // Return node candidates for stop s from this group
    const router::node_candidate_group& get_node_candidates(const gtfs::stop* s) const;

    // Write the penalties for all stops contained in this group so far.
    void write_penalties(const trgraph::normalizer& platformNorm, double trackPen,
                   double distPenFac, double nonOsmPen);

private:
    std::set<trgraph::node*> _nodes;
    std::set<const gtfs::stop*> _stops;

    // for each stop in this group, a penalty for each of the nodes here, based on
    // its distance and optionally the track number
    std::unordered_map<const gtfs::stop*, router::node_candidate_group> _stopNodePens;

    double get_penalty(const gtfs::stop* s, trgraph::node* n,
                  const trgraph::normalizer& norm, double trackPen,
                  double distPenFac, double nonOsmPen) const;
};
}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_STATGROUP_H_
