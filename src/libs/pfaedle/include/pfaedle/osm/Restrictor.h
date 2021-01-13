// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_RESTRICTOR_H_
#define PFAEDLE_OSM_RESTRICTOR_H_

#include "pfaedle/osm/Osm.h"
#include "pfaedle/trgraph/graph.h"

#include <map>
#include <vector>
#include <utility>
#include <unordered_map>

namespace pfaedle::osm
{

using RulePair = std::pair<const trgraph::edge*, const trgraph::edge*>;
using RuleVec = std::vector<RulePair>;
using DanglPath = std::pair<const trgraph::node*, size_t>;
// very seldom, there are more than a handful of rules for a node. Use a
// vector here, should have lesser overhead and be faster for such small
// numbers
using Rules = std::unordered_map<const trgraph::node*, RuleVec>;
using NodeOsmIdP = std::pair<const trgraph::node*, osmid>;

/*
 * Stores restrictions between edges
 */
class Restrictor
{
public:
    Restrictor() = default;

    void relax(osmid wid,
               const trgraph::node* n,
               const trgraph::edge* e);
    void add(const trgraph::edge* from,
             osmid to,
             const trgraph::node* via,
             bool pos);
    bool may(const trgraph::edge* from, const trgraph::edge* to,
             const trgraph::node* via) const;
    void replaceEdge(const trgraph::edge* old,
                     const trgraph::edge* newA,
                     const trgraph::edge* newB);
    void duplicateEdge(const trgraph::edge* old,
                       const trgraph::node* via,
                       const trgraph::edge* newE);
    void duplicateEdge(const trgraph::edge* old,
                       const trgraph::edge* newE);

private:
    void replaceEdge(const trgraph::edge* old,
                     const trgraph::node* via,
                     const trgraph::edge* newE);

    Rules _pos;
    Rules _neg;

    std::map<NodeOsmIdP, const trgraph::edge*> _rlx;

    std::map<NodeOsmIdP, std::vector<DanglPath>> _posDangling;
    std::map<NodeOsmIdP, std::vector<DanglPath>> _negDangling;

};
}  // namespace pfaedle::osm

#endif  // PFAEDLE_OSM_RESTRICTOR_H_
