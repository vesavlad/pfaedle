#ifndef CONFIG_FILE_PARSER_H_OSM_READER_H
#define CONFIG_FILE_PARSER_H_OSM_READER_H

#include <vector>
#include <pfaedle/router/misc.h>
#include <pfaedle/osm/osm.h>
#include <pfaedle/osm/osm_read_options.h>
#include <pfaedle/osm/osm_id_set.h>
#include <pfaedle/osm/osm_filter.h>
#include <pfaedle/trgraph/restrictor.h>
#include <pfaedle/trgraph/station_group.h>

namespace pfaedle::osm
{
    class bounding_box;
    class relation_handler;
    class way_handler;
    class node_handler;

    class osm_reader
    {
        friend class relation_handler;
        friend class way_handler;
        friend class node_handler;

    public:
        struct read_configuration
        {
            const osm_read_options &options;
            trgraph::graph &graph;
            trgraph::restrictor &restrictor;
        };

        explicit osm_reader(const osm_reader::read_configuration &configuration);

        void read(const std::string &path, const bounding_box &bbox);


        const router::node_set &get_orphan_stations() const
        {
            return orphan_stations;
        }

    private:
        static std::optional<trgraph::station_info> get_station_info(trgraph::node *node,
                                                                     osmid nid,
                                                                     const POINT &pos,
                                                                     const attribute_map &m,
                                                                     station_attribute_groups *groups,
                                                                     const relation_map &nodeRels,
                                                                     const relation_list &rels,
                                                                     const osm_read_options &ops);


        static std::string get_attribute(const deep_attribute_rule &s,
                                         osmid id,
                                         const attribute_map &attrs,
                                         const relation_map &entRels,
                                         const relation_list &rels);

        static std::string get_attribute_by_first_match(const deep_attribute_list &rule,
                                                        osmid id,
                                                        const attribute_map &attrs,
                                                        const relation_map &entRels,
                                                        const relation_list &rels,
                                                        const trgraph::normalizer &normzer);

        static std::vector<std::string> get_matching_attributes_ranked(const deep_attribute_list &rule,
                                                                       osmid id,
                                                                       const attribute_map &attrs,
                                                                       const relation_map &entRels,
                                                                       const relation_list &rels,
                                                                       const trgraph::normalizer &normalizer);

        std::vector<trgraph::transit_edge_line *> get_lines(const std::vector<size_t> &edgeRels,
                                                            const relation_list &rels,
                                                            const osm_read_options &ops);


        void process_restrictions(osmid nid,
                                  osmid wid,
                                  trgraph::edge *e,
                                  trgraph::node *n) const;

    private:

        const read_configuration &configuration;

        router::node_set orphan_stations;
        edge_tracks e_tracks;
        osm_id_set usable_node_set;
        osm_id_set restricted_node_set;

        node_id_map nodes;
        node_id_multimap nodes_multi_map;

        relation_list intermodal_rels;
        relation_map node_rels;
        relation_map way_rels;

        restrictions raw_rests;

        std::vector<attribute_key_set> kept_attribute_keys;

        osm_filter filter;


        std::map<trgraph::transit_edge_line, trgraph::transit_edge_line *> _lines;
        std::map<size_t, trgraph::transit_edge_line *> _relLines;
    };


}

#endif //CONFIG_FILE_PARSER_H_OSM_READER_H
