// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_OSMBUILDER_H_
#define PFAEDLE_OSM_OSMBUILDER_H_

#include <pfaedle/definitions.h>
#include <pfaedle/osm/bounding_box.h>
#include <pfaedle/osm/osm_filter.h>
#include <pfaedle/osm/osm_id_set.h>
#include <pfaedle/osm/osm_read_options.h>
#include <pfaedle/osm/restrictor.h>
#include <pfaedle/router/router.h>
#include <pfaedle/trgraph/graph.h>
#include <pfaedle/trgraph/node_payload.h>
#include <pfaedle/trgraph/normalizer.h>
#include <pfaedle/trgraph/station_info.h>
#include <util/geo/Geo.h>
#include <util/xml/XmlWriter.h>

#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace pugi
{
class xml_document;
class xml_node;
}  // namespace pugi

namespace pfaedle::gtfs
{
struct stop;
}
namespace pfaedle::osm
{

struct NodeCand
{
    double dist;
    trgraph::node* node;
    const trgraph::edge* fromEdge;
    int fullTurns;
};

struct search_functor
{
    virtual bool operator()(const trgraph::node* n, const trgraph::station_info* si) const = 0;
};

struct eq_search_functor : public search_functor
{
    explicit eq_search_functor(bool orphan_snap) :
        orphanSnap(orphan_snap) {}
    double minSimi = 0.9;
    bool orphanSnap;
    bool operator()(const trgraph::node* cand, const trgraph::station_info* si) const override;
};

struct block_search_functor : public search_functor
{
    bool operator()(const trgraph::node* n, const trgraph::station_info* si) const override
    {
        if (n->pl().get_si() && n->pl().get_si()->simi(si) < 0.5) return true;
        return n->pl().is_blocker();
    }
};

inline bool operator<(const NodeCand& a, const NodeCand& b)
{
    return a.fullTurns > b.fullTurns || a.dist > b.dist;
}

using node_candidate_priority_queue = std::priority_queue<NodeCand>;

/*
 * Builds a physical transit network graph from OSM data
 */
class osm_builder
{
public:
    osm_builder();

    // Read the OSM file at path, and write a graph to g. Only elements
    // inside the bounding box will be read
    void read(const std::string& path,
              const osm_read_options& opts,
              trgraph::graph& g,
              const bounding_box& box,
              size_t gridSize,
              router::feed_stops& fs,
              restrictor& res,
              bool import_osm_stations);

    // Based on the list of options, output an overpass XML query for getting
    // the data needed for routing
    void overpass_query_write(std::ostream& out,
                              const std::vector<osm_read_options>& opts,
                              const bounding_box& latLngBox) const;

    // Based on the list of options, read an OSM file from in and output an
    // OSM file to out which contains exactly the entities that are needed
    // from the file at in
    void filter_write(const std::string& in,
                      const std::string& out,
                      const std::vector<osm_read_options>& opts,
                      const bounding_box& box);

private:
    int filter_nodes(pugi::xml_document& xml, osm_id_set& nodes,
                     osm_id_set& noHupNodes, const osm_filter& filter,
                     const bounding_box& bbox) const;

    void read_relations(pugi::xml_document& xml,
                        relation_list& rels,
                        relation_map& nodeRels,
                        relation_map& wayRels,
                        const osm_filter& filter,
                        const attribute_key_set& keepAttrs,
                        restrictions& rests) const;

    void read_restrictions(const osm_relation& rel,
                           restrictions& rests,
                           const osm_filter& filter) const;

    void read_nodes(pugi::xml_document& xml,
                    trgraph::graph& g,
                    const relation_list& rels,
                    const relation_map& nodeRels,
                    const osm_filter& filter,
                    const osm_id_set& bBoxNodes,
                    node_id_map& nodes,
                    node_id_multimap& multNodes,
                    router::node_set& orphanStations,
                    const attribute_key_set& keepAttrs,
                    const flat_relations& fl,
                    const osm_read_options& opts) const;

    void read_write_nodes(pugi::xml_document& i,
                          pugi::xml_node& o,
                          const relation_map& nRels,
                          const osm_filter& filter,
                          const osm_id_set& bBoxNds,
                          node_id_map& nds,
                          const attribute_key_set& keepAttrs,
                          const flat_relations& f) const;

    void read_write_ways(pugi::xml_document& i,
                         pugi::xml_node& o,
                         osmid_list& ways,
                         const attribute_key_set& keepAttrs) const;

    void read_write_relations(pugi::xml_document& i,
                              pugi::xml_node& o,
                              osmid_list& ways,
                              node_id_map& nodes,
                              const osm_filter& filter,
                              const attribute_key_set& keepAttrs);

    void read_edges(pugi::xml_document& xml,
                    trgraph::graph& g, const relation_list& rels,
                    const relation_map& wayRels,
                    const osm_filter& filter,
                    const osm_id_set& bBoxNodes,
                    node_id_map& nodes,
                    node_id_multimap& multNodes,
                    const osm_id_set& noHupNodes,
                    const attribute_key_set& keepAttrs,
                    const restrictions& rest,
                    restrictor& restor,
                    const flat_relations& flatRels,
                    edge_tracks& etracks,
                    const osm_read_options& opts);

    void read_ways(pugi::xml_document& xml,
                    const relation_map& wayRels,
                    const osm_filter& filter,
                    const osm_id_set& bBoxNodes,
                    const attribute_key_set& keepAttrs,
                    osmid_list& ret,
                    node_id_map& nodes,
                    const flat_relations& flatRels);

    bool keep_way(const osm_way& w, const relation_map& wayRels, const osm_filter& filter,
                  const osm_id_set& bBoxNodes, const flat_relations& fl) const;


    bool keep_node(const osm_node& n, const node_id_map& nodes,
                   const node_id_multimap& multNodes, const relation_map& nodeRels,
                   const osm_id_set& bBoxNodes, const osm_filter& filter,
                   const flat_relations& fl) const;

    std::optional<trgraph::station_info> get_station_info(trgraph::node* node, osmid nid, const POINT& pos,
                                                       const attribute_map& m, station_attribute_groups* groups,
                                                       const relation_map& nodeRels, const relation_list& rels,
                                                       const osm_read_options& ops) const;

    static void snap_stations(const osm_read_options& opts,
                           trgraph::graph& g,
                           const bounding_box& bbox,
                           size_t gridSize,
                           router::feed_stops& fs,
                           restrictor& res,
                           const router::node_set& orphanStations,
                              const bool import_osm_stations);
    static double webMercDist(const trgraph::node& a, const trgraph::node& b);

    static void writeODirEdgs(trgraph::graph& g, restrictor& restor);
    static void write_edge_tracks(const edge_tracks& tracks);

    static router::node_set snap_station(trgraph::graph& g,
                                         trgraph::node_payload& s,
                                         trgraph::edge_grid& eg,
                                         trgraph::node_grid& sng,
                                         const osm_read_options& opts,
                                         restrictor& restor,
                                         bool surHeur,
                                         bool orphSnap,
                                         double maxD,
                                         bool import_osm_stations);

    // Checks if from the edge e, a station similar to si can be reach with less
    // than maxD distance and less or equal to "maxFullTurns" full turns. If
    // such a station exists, it is returned. If not, 0 is returned.
    static trgraph::node* eqStatReach(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
                                      double maxD, int maxFullTurns, double maxAng,
                                      bool orph);

    static trgraph::node* depth_search(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
                                       double maxD, int maxFullTurns, double minAngle,
                                       const search_functor& sfunc);

    static bool is_blocked(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
                           double maxD, int maxFullTurns, double minAngle);

    static trgraph::station_group* group_stats(const router::node_set& s);

    static trgraph::node_payload payload_from_gtfs(const pfaedle::gtfs::stop* s, const osm_read_options& ops);

    std::vector<trgraph::transit_edge_line*> get_lines(const std::vector<size_t>& edgeRels,
                                                       const relation_list& rels,
                                                       const osm_read_options& ops);

    void process_restrictions(osmid nid, osmid wid, const restrictions& rawRests, trgraph::edge* e,
                              trgraph::node* n, restrictor& restor) const;

    std::string getAttrByFirstMatch(const deep_attribute_list& rule, osmid id,
                                    const attribute_map& attrs, const relation_map& entRels,
                                    const relation_list& rels,
                                    const trgraph::normalizer& norm) const;

    std::vector<std::string> getAttrMatchRanked(const deep_attribute_list& rule, osmid id,
                                                const attribute_map& attrs,
                                                const relation_map& entRels,
                                                const relation_list& rels,
                                                const trgraph::normalizer& norm) const;

    std::string get_attribute(const deep_attribute_rule& s, osmid id, const attribute_map& attrs,
                        const relation_map& entRels, const relation_list& rels) const;

    bool should_keep_relation(osmid id, const relation_map& rels, const flat_relations& fl) const;

    std::map<trgraph::transit_edge_line, trgraph::transit_edge_line*> _lines;
    std::map<size_t, trgraph::transit_edge_line*> _relLines;
};
}  // namespace pfaedle::osm
#endif  // PFAEDLE_OSM_OSMBUILDER_H_
