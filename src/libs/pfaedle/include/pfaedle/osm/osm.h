// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_OSM_H_
#define PFAEDLE_OSM_OSM_H_

#include <util/graph/Node.h>

#include <set>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace pfaedle
{
    namespace trgraph
    {
        class node_payload;
        class edge_payload;
        using node = util::graph::Node<node_payload, edge_payload>;
    }

    namespace osm
    {

        using osmid = uint64_t;

        using node_id_map = std::unordered_map<osmid, trgraph::node *>;
        using node_id_multimap = std::unordered_map<osmid, std::set<trgraph::node *>>;
        using attribute_map = std::unordered_map<std::string, std::string>;
        using attribute = std::pair<std::string, std::string>;
        using osmid_list = std::vector<osmid>;

        using relation_map = std::unordered_map<osmid, std::vector<size_t>>;
        using flat_relations = std::set<size_t>;

        class osm_filter;

        class osm_id_set;

        struct osm_element
        {
            osmid id;
            attribute_map attrs;

        protected:
            bool should_keep_relation(const relation_map &rels, const flat_relations &fl) const;
        };

        struct osm_relation : public osm_element
        {
            std::vector<osmid> nodes;
            std::vector<osmid> ways;

            std::vector<std::string> nodeRoles;
            std::vector<std::string> wayRoles;

            uint64_t keepFlags;
            uint64_t dropFlags;
        };

        struct osm_way : public osm_element
        {
            std::vector<osmid> nodes;

            uint64_t keepFlags;
            uint64_t dropFlags;

            bool keep_way(const relation_map &wayRels,
                          const osm_filter &filter,
                          const osm_id_set &bBoxNodes,
                          const flat_relations &fl) const;
        };

        struct osm_node : public osm_element
        {
            double lat;
            double lng;

            uint64_t keepFlags;
            uint64_t dropFlags;

            bool keep_node(const node_id_map &nodes,
                           const node_id_multimap &multNodes,
                           const relation_map &nodeRels,
                           const osm_id_set &bBoxNodes,
                           const osm_filter &filter,
                           const flat_relations &fl) const;
        };

        struct restriction
        {
            restriction(osmid from, osmid to) :
                    eFrom{from},
                    eTo{to} {}

            osmid eFrom;
            osmid eTo;
        };

        using restriction_map = std::unordered_map<osmid, std::vector<restriction>>;

        struct restrictions
        {
            restriction_map pos;
            restriction_map neg;
        };
    }
}  // namespace pfaedle::osm
#endif  // PFAEDLE_OSM_OSM_H_
