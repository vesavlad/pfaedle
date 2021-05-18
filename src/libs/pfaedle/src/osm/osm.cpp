#include "pfaedle/osm/osm.h"

#include <pfaedle/osm/osm_filter.h>
#include <pfaedle/osm/osm_id_set.h>

namespace pfaedle::osm
{

    bool osm_element::should_keep_relation(const relation_map &rels, const flat_relations &fl) const
    {
        auto it = rels.find(id);

        if (it == rels.end())
            return false;
        return std::any_of(it->second.begin(), it->second.end(), [&fl](auto rel_id) {
            // as soon as any of this entities relations is not flat, return true
            return !fl.count(rel_id);
        });
    }

    bool osm_way::keep_way(const relation_map &wayRels, const osm_filter &filter,
                           const osm_id_set &bBoxNodes, const flat_relations &fl) const
    {
        if (id && nodes.size() > 1 &&
            (should_keep_relation(wayRels, fl) || filter.keep(attrs, osm_filter::WAY)) &&
            !filter.drop(attrs, osm_filter::WAY)) {
            for (osmid nid : nodes) {
                if (bBoxNodes.has(nid)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool osm_node::keep_node(const node_id_map &nodes, const node_id_multimap &multNodes, const relation_map &nodeRels,
                             const osm_id_set &bBoxNodes, const osm_filter &filter, const flat_relations &fl) const
    {
        if (id &&
            (nodes.count(id) || multNodes.count(id) || should_keep_relation(nodeRels, fl) ||
             filter.keep(attrs, osm_filter::NODE)) &&
            (nodes.count(id) || bBoxNodes.has(id)) &&
            (nodes.count(id) || multNodes.count(id) || !filter.drop(attrs, osm_filter::NODE))) {
            return true;
        }

        return false;
    }
}