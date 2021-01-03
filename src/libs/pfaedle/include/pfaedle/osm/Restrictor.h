// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_RESTRICTOR_H_
#define PFAEDLE_OSM_RESTRICTOR_H_

#include "pfaedle/osm/Osm.h"
#include "pfaedle/trgraph/Graph.h"

#include <map>
#include <vector>
#include <utility>
#include <unordered_map>

namespace pfaedle::osm
{

using RulePair = std::pair<const trgraph::Edge*, const trgraph::Edge*>;
using RuleVec = std::vector<RulePair>;
using DanglPath = std::pair<const trgraph::Node*, size_t>;
// very seldom, there are more than a handful of rules for a node. Use a
// vector here, should have lesser overhead and be faster for such small
// numbers
using Rules = std::unordered_map<const trgraph::Node*, RuleVec>;
using NodeOsmIdP = std::pair<const trgraph::Node*, osmid>;

/*
 * Stores restrictions between edges
 */
class Restrictor
{
public:
    Restrictor() = default;

    void relax(osmid wid,
               const trgraph::Node* n,
               const trgraph::Edge* e);
    void add(const trgraph::Edge* from,
             osmid to,
             const trgraph::Node* via,
             bool pos);
    bool may(const trgraph::Edge* from, const trgraph::Edge* to,
             const trgraph::Node* via) const;
    void replaceEdge(const trgraph::Edge* old,
                     const trgraph::Edge* newA,
                     const trgraph::Edge* newB);
    void duplicateEdge(const trgraph::Edge* old,
                       const trgraph::Node* via,
                       const trgraph::Edge* newE);
    void duplicateEdge(const trgraph::Edge* old,
                       const trgraph::Edge* newE);

private:
    void replaceEdge(const trgraph::Edge* old,
                     const trgraph::Node* via,
                     const trgraph::Edge* newE);

    Rules _pos;
    Rules _neg;

    std::map<NodeOsmIdP, const trgraph::Edge*> _rlx;

    std::map<NodeOsmIdP, std::vector<DanglPath>> _posDangling;
    std::map<NodeOsmIdP, std::vector<DanglPath>> _negDangling;

};
}  // namespace pfaedle::osm

#endif  // PFAEDLE_OSM_RESTRICTOR_H_
