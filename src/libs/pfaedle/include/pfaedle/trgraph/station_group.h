// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_STATGROUP_H_
#define PFAEDLE_TRGRAPH_STATGROUP_H_

#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/router/router.h"
#include "pfaedle/trgraph/graph.h"
#include "pfaedle/trgraph/normalizer.h"
#include <set>
#include <string>
#include <unordered_map>

namespace pfaedle::trgraph
{

using ad::cppgtfs::gtfs::Stop;

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
    void addStop(const Stop* s);

    // Add a node n to this station group
    void addNode(trgraph::node* n);

    // Return all nodes contained in this group
    const std::set<trgraph::node*>& getNodes() const;
    std::set<trgraph::node*>& getNodes();

    // Return all stops contained in this group
    const std::set<const Stop*>& getStops() const;

    // Remove a node from this group
    void remNode(trgraph::node* n);

    // All nodes in other will be in this group, their SI's updated, and the
    // "other" group deleted.
    void merge(station_group* other);

    // Return node candidates for stop s from this group
    const router::node_candidate_group& getNodeCands(const Stop* s) const;

    // Write the penalties for all stops contained in this group so far.
    void writePens(const trgraph::normalizer& platformNorm, double trackPen,
                   double distPenFac, double nonOsmPen);

private:
    std::set<trgraph::node*> _nodes;
    std::set<const Stop*> _stops;

    // for each stop in this group, a penalty for each of the nodes here, based on
    // its distance and optionally the track number
    std::unordered_map<const Stop*, router::node_candidate_group> _stopNodePens;

    double getPen(const Stop* s, trgraph::node* n,
                  const trgraph::normalizer& norm, double trackPen,
                  double distPenFac, double nonOsmPen) const;
};
}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_STATGROUP_H_
