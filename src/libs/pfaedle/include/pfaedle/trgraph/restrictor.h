// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_RESTRICTOR_H_
#define PFAEDLE_OSM_RESTRICTOR_H_

#include "pfaedle/osm/osm.h"
#include "pfaedle/trgraph/graph.h"

#include <map>
#include <vector>
#include <utility>
#include <unordered_map>

namespace pfaedle::trgraph
{

using RulePair = std::pair<const trgraph::edge*, const trgraph::edge*>;
using RuleVec = std::vector<RulePair>;
// very seldom, there are more than a handful of rules for a node. Use a
// vector here, should have lesser overhead and be faster for such small
// numbers
using Rules = std::unordered_map<const trgraph::node*, RuleVec>;

using osmid = uint64_t;

/*
 * Stores restrictions between edges
 */
class restrictor
{
public:
    restrictor() = default;

    void relax(osmid wid,
               const trgraph::node* n,
               const trgraph::edge* e);
    void add(const trgraph::edge* from,
             osmid to,
             const trgraph::node* via,
             bool pos);
    bool may(const trgraph::edge* from,
             const trgraph::edge* to,
             const trgraph::node* via) const;

    void replace_edge(const trgraph::edge* old,
                      const trgraph::edge* newA,
                      const trgraph::edge* newB);
    void duplicate_edge(const trgraph::edge* old,
                        const trgraph::node* via,
                        const trgraph::edge* newE);
    void duplicate_edge(const trgraph::edge* old,
                        const trgraph::edge* newE);

private:
    using dangling_path = std::pair<const trgraph::node*, size_t>;
    using node_id_pair = std::pair<const trgraph::node*, osmid>;

    void replace_edge(const trgraph::edge* old,
                      const trgraph::node* via,
                      const trgraph::edge* newE);

    Rules _pos;
    Rules _neg;

    std::map<node_id_pair, const trgraph::edge*> _rlx;

    std::map<node_id_pair, std::vector<dangling_path>> _posDangling;
    std::map<node_id_pair, std::vector<dangling_path>> _negDangling;
};
}  // namespace pfaedle::trgraph

#endif  // PFAEDLE_OSM_RESTRICTOR_H_
