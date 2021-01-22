// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/osm/osm_builder.h"
#include "pfaedle/definitions.h"
#include "pfaedle/osm/bounding_box.h"
#include "pfaedle/osm/osm.h"
#include "pfaedle/osm/osm_filter.h"
#include "pfaedle/trgraph/restrictor.h"
#include "pfaedle/trgraph/station_group.h"
#include "util/Misc.h"
#include "util/String.h"

#include <logging/logger.h>
#include <pugixml.hpp>
#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <fstream>

using pfaedle::trgraph::component;
using pfaedle::trgraph::edge;
using pfaedle::trgraph::edge_payload;
using pfaedle::trgraph::graph;
using pfaedle::trgraph::node;
using pfaedle::trgraph::node_payload;
using pfaedle::trgraph::normalizer;
using pfaedle::trgraph::station_info;
using pfaedle::trgraph::transit_edge_line;
using pfaedle::trgraph::transit_edge_line;
using util::geo::webMercMeterDist;
using util::geo::Point;

namespace pfaedle::osm
{

/**
 * @return the speed in km/h
 */
inline double string_to_kmh(std::string str)
{
    /**
     * The speed value used for "none" speed limit on German Autobahn is 150=30*5 as this is the biggest value
     * not explicitly used in OSM and can be precisely returned for a factor of 5, 3, 2 and 1. It is fixed and
     * not DecimalEncodedValue.getMaxInt to allow special case handling.
     */
    static constexpr double UNLIMITED_SIGN_SPEED = 150;

    if (str.empty() || util::trim(str).empty())
        return 50;

    // on some German autobahns and a very few other places
    if ("none" == str)
        return UNLIMITED_SIGN_SPEED;

    if (util::ends_with(str, ":rural") || util::ends_with(str, ":trunk"))
        return 80;

    if (util::ends_with(str, ":urban"))
        return 50;

    if (str == "walk" || util::ends_with(str, ":living_street"))
        return 6;

    size_t mpInteger = str.find("mp");
    size_t knotInteger = str.find("knots");
    size_t kmInteger = str.find("km");
    size_t kphInteger = str.find("kph");

    double factor;
    if (mpInteger != std::string::npos)
    {
        str = util::trim(str.substr(0, mpInteger));
        // factor = DistanceCalcEarth.KM_MILE;
        factor = 1.609344f;
    }
    else if (knotInteger != std::string::npos)
    {
        str = util::trim(str.substr(0, knotInteger));
        factor = 1.852;// see https://en.wikipedia.org/wiki/Knot_%28unit%29#Definitions
    }
    else
    {
        if (kmInteger != std::string::npos)
        {
            str = util::trim(str.substr(0, kmInteger));
        }
        else if (kphInteger != std::string::npos)
        {
            str = util::trim(str.substr(0, kphInteger));
        }
        factor = 1;
    }

    double value = stoi(str) * factor;

    if (value <= 0)
    {
        return 50;
    }

    return value;
}

bool eq_search_functor::operator()(const node* cand, const station_info* si) const
{
    if (orphanSnap && cand->pl().get_si() &&
        (!cand->pl().get_si()->get_group() ||
         cand->pl().get_si()->get_group()->get_stops().empty()))
    {
        return true;
    }
    return cand->pl().get_si() && cand->pl().get_si()->simi(si) > minSimi;
}


osm_builder::osm_builder() = default;


void osm_builder::read(const std::string& path,
                       const osm_read_options& opts,
                       graph& g,
                       const bounding_box& bbox,
                       size_t gridSize,
                       router::feed_stops& fs,
                       trgraph::restrictor& res,
                       bool import_osm_stations)
{
    if (!bbox.size())
        return;

    LOG(INFO) << "Reading OSM file " << path << " ... ";

    router::node_set orphan_stations;
    edge_tracks e_tracks;
    {
        osm_id_set bboxNodes, noHupNodes;

        node_id_map nodes;
        node_id_multimap mult_nodes;
        relation_list intm_rels;
        relation_map node_rels, wayRels;

        restrictions raw_rests;

        std::vector<attribute_key_set> attr_keys = opts.get_kept_attribute_keys();

        osm_filter filter(opts);

        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(path.c_str());
        if (!result)
            return;

        // we do four passes of the file here to be as memory creedy as possible:
        // - the first pass collects all node IDs which are
        //    * inside the given bounding box
        //    * (TODO: maybe more filtering?)
        //   these nodes are stored on the HD via OsmIdSet (which implements a
        //   simple bloom filter / base 256 encoded id store
        // - the second pass collects filtered relations
        // - the third pass collects filtered ways which contain one of the nodes
        //   from pass 1
        // - the forth pass collects filtered nodes which were
        //    * collected as node ids in pass 1
        //    * match the filter criteria
        //    * have been used in a way in pass 3

        LOG(TRACE) << "Reading bounding box nodes...";
        filter_nodes(doc, bboxNodes, noHupNodes, filter, bbox);

        LOG(TRACE) << "Reading relations...";
        read_relations(doc, intm_rels, node_rels, wayRels, filter, attr_keys[2], raw_rests);

        LOG(TRACE) << "Reading edges...";
        read_edges(doc, g, intm_rels, wayRels, filter, bboxNodes, nodes, mult_nodes,
                   noHupNodes, attr_keys[1], raw_rests, res, intm_rels.flat, e_tracks,
                   opts);

        LOG(TRACE) << "Reading kept nodes...";
        read_nodes(doc, g, intm_rels, node_rels, filter, bboxNodes, nodes,
                   mult_nodes, orphan_stations, attr_keys[0], intm_rels.flat, opts);
    }

    LOG(TRACE) << "OSM ID set lookups: " << osm::osm_id_set::LOOKUPS
               << ", file lookups: " << osm::osm_id_set::FLOOKUPS;

    LOG(TRACE) << "Applying edge track numbers...";
    write_edge_tracks(e_tracks);
    e_tracks.clear();

    {
        LOG(TRACE) << "Fixing gaps...";
        trgraph::node_grid ng = trgraph::node_grid::build_node_grid(g, gridSize, bbox.get_full_web_merc_box(), false);
        g.fix_gaps(ng);
    }

    LOG(TRACE) << "Writing edge geoms...";
    g.write_geometries();

    LOG(TRACE) << "Snapping stations...";
    snap_stations(opts, g, bbox, gridSize, fs, res, orphan_stations, import_osm_stations);

    LOG(TRACE) << "Deleting orphan nodes...";
    g.delete_orphan_nodes();

    LOG(TRACE) << "Deleting orphan edges...";
    g.delete_orphan_edges(opts.fullTurnAngle);

    LOG(TRACE) << "Collapsing edges...";
    g.collapse_edges();

    LOG(TRACE) << "Deleting orphan nodes...";
    g.delete_orphan_nodes();

    LOG(TRACE) << "Deleting orphan edges...";
    g.delete_orphan_edges(opts.fullTurnAngle);

    LOG(TRACE) << "Writing graph components...";
    // the restrictor is needed here to prevent connections in the graph
    // which are not possible in reality
    uint32_t comps = g.write_components();

    LOG(TRACE) << "Simplifying geometries...";
    g.simplify_geometries();

    LOG(TRACE) << "Writing other-direction edges...";
    g.writeODirEdgs(res);

    LOG(TRACE) << "Write dummy node self-edges...";
    g.writeSelfEdgs();

    size_t num_edges = 0;

    for (const auto& n : g.getNds())
    {
        num_edges += n->getAdjListOut().size();
    }

    LOG(DEBUG) << "Graph has " << g.getNds().size() << " nodes, " << num_edges
               << " edges and " << comps << " connected component(s)";
}


void osm_builder::overpass_query_write(std::ostream& out,
                                       const std::vector<osm_read_options>& opts,
                                       const bounding_box& latLngBox) const
{
    osm_id_set bboxNodes;
    osm_id_set noHupNodes;
    multi_attribute_map emptyF;

    relation_list rels;
    osmid_list ways;
    relation_map nodeRels, wayRels;

    // TODO(patrick): not needed here!
    restrictions rests;

    node_id_map nodes;

    // always empty
    node_id_multimap multNodes;
    util::xml::XmlWriter wr(out, true, 4);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    wr.openComment();
    wr.writeText(" - written by pfaedle -");
    wr.closeTag();
    wr.openTag("osm-script");

    osm_filter filter;

    for (const osm_read_options& o : opts)
    {
        filter = filter.merge(osm_filter(o.keepFilter, o.dropFilter));
    }

    wr.openTag("union");
    size_t c = 0;
    for (auto box : latLngBox.get_leafs())
    {
        if (box.getLowerLeft().getX() > box.getUpperRight().getX()) continue;
        c++;
        wr.openComment();
        wr.writeText(std::string("Bounding box #") + std::to_string(c) + " (" +
                     std::to_string(box.getLowerLeft().getY()) + ", " +
                     std::to_string(box.getLowerLeft().getX()) + ", " +
                     std::to_string(box.getUpperRight().getY()) + ", " +
                     std::to_string(box.getUpperRight().getX()) + ")");
        wr.closeTag();
        for (const auto& t : std::vector<std::string>{"way", "node", "relation"})
        {
            for (const auto& r : filter.getKeepRules())
            {
                for (const auto& val : r.second)
                {
                    if (t == "way" && (val.second & osm_filter::WAY)) continue;
                    if (t == "relation" && (val.second & osm_filter::REL)) continue;
                    if (t == "node" && (val.second & osm_filter::NODE)) continue;

                    wr.openTag("query", {{"type", t}});
                    if (val.first == "*")
                        wr.openTag("has-kv", {{"k", r.first}});
                    else
                        wr.openTag("has-kv", {{"k", r.first}, {"v", val.first}});
                    wr.closeTag();
                    wr.openTag("bbox-query",
                               {{"s", std::to_string(box.getLowerLeft().getY())},
                                {"w", std::to_string(box.getLowerLeft().getX())},
                                {"n", std::to_string(box.getUpperRight().getY())},
                                {"e", std::to_string(box.getUpperRight().getX())}});
                    wr.closeTag();
                    wr.closeTag();
                }
            }
        }
    }

    wr.closeTag();

    wr.openTag("union");
    wr.openTag("item");
    wr.closeTag();
    wr.openTag("recurse", {{"type", "down"}});
    wr.closeTag();
    wr.closeTag();
    wr.openTag("print");

    wr.closeTags();
}


void osm_builder::filter_write(const std::string& in,
                               const std::string& out,
                               const std::vector<osm_read_options>& opts,
                               const bounding_box& box)
{
    osm_id_set bboxNodes, noHupNodes;
    multi_attribute_map emptyF;

    relation_list rels;
    osmid_list ways;
    relation_map nodeRels, wayRels;

    // TODO(patrick): not needed here!
    restrictions rests;

    node_id_map nodes;

    // always empty
    node_id_multimap multNodes;

    pugi::xml_document input_doc;
    input_doc.load_file(in.c_str());

    pugi::xml_document outout_doc;
    auto osm_child = outout_doc.append_child("osm");
    auto bounds_child = osm_child.append_child("bounds");
    bounds_child.append_attribute("minlat").set_value(box.get_full_box().getLowerLeft().getY());
    bounds_child.append_attribute("minlon").set_value(box.get_full_box().getLowerLeft().getX());
    bounds_child.append_attribute("maxlat").set_value(box.get_full_box().getUpperRight().getY());
    bounds_child.append_attribute("maxlon").set_value(box.get_full_box().getUpperRight().getX());


    osm_filter filter;
    std::vector<attribute_key_set> attr_keys(3);

    for (const osm_read_options& o : opts)
    {
        auto result = o.get_kept_attribute_keys();
        for (size_t i = 0; i < attr_keys.size(); ++i)
        {
            attr_keys[i].merge(result[i]);
        }
        filter = filter.merge(osm_filter(o.keepFilter, o.dropFilter));
    }

    filter_nodes(input_doc, bboxNodes, noHupNodes, filter, box);

    read_relations(input_doc, rels, nodeRels, wayRels, filter, attr_keys[2], rests);
    read_ways(input_doc, wayRels, filter, bboxNodes, attr_keys[1], ways, nodes, rels.flat);

    read_write_nodes(input_doc, osm_child, nodeRels, filter, bboxNodes, nodes, attr_keys[0], rels.flat);
    read_write_ways(input_doc, osm_child, ways, attr_keys[1]);

    std::sort(ways.begin(), ways.end());
    read_write_relations(input_doc, osm_child, ways, nodes, filter, attr_keys[2]);

    std::ofstream outstr;
    outstr.open(out);
    outout_doc.save(outstr);
}


void osm_builder::read_write_relations(pugi::xml_document& i,
                                       pugi::xml_node& o,
                                       osmid_list& ways,
                                       node_id_map& nodes,
                                       const osm_filter& filter,
                                       const attribute_key_set& keepAttrs)
{
    for (const auto& xmlrel : i.child("osm").children("relation"))
    {
        osm_relation rel;
        uint64_t keep_flags = 0;
        uint64_t drop_flags = 0;

        rel.id = xmlrel.attribute("id").as_ullong();
        // processing attributes
        {
            for (const auto& tag : xmlrel.children("tag"))
            {
                const auto& key = tag.attribute("k").as_string();
                const auto& value = tag.attribute("v").as_string();
                if (keepAttrs.count(key))
                {
                    rel.attrs[key] = value;
                }
            }

            if (rel.id && !rel.attrs.empty() &&
                (keep_flags = filter.keep(rel.attrs, osm_filter::REL)) &&
                !(drop_flags = filter.drop(rel.attrs, osm_filter::REL)))
            {
                rel.keepFlags = keep_flags;
                rel.dropFlags = drop_flags;
            }
        }

        // processing members
        for (const auto& member : xmlrel.children("member"))
        {
            const auto& type = member.attribute("type").as_string();
            if (strcmp(type, "node") == 0)
            {
                osmid id = member.attribute("ref").as_ullong();
                rel.nodes.emplace_back(id);
                rel.nodeRoles.emplace_back(member.attribute("role").as_string());
            }
            if (strcmp(type, "way") == 0)
            {
                osmid id = member.attribute("ref").as_ullong();
                rel.ways.emplace_back(id);
                rel.wayRoles.emplace_back(member.attribute("role").as_string());
            }
        }


        osmid_list realNodes;
        osmid_list realWays;
        std::vector<const char*> realNodeRoles;
        std::vector<const char*> realWayRoles;

        for (size_t j = 0; j < rel.ways.size(); j++)
        {
            osmid wid = rel.ways[j];
            const auto& i = std::lower_bound(ways.begin(), ways.end(), wid);
            if (i != ways.end() && *i == wid)
            {
                realWays.push_back(wid);
                realWayRoles.push_back(rel.wayRoles[j].c_str());
            }
        }

        for (size_t j = 0; j < rel.nodes.size(); j++)
        {
            osmid nid = rel.nodes[j];
            if (nodes.count(nid))
            {
                realNodes.push_back(nid);
                realNodeRoles.push_back(rel.nodeRoles[j].c_str());
            }
        }

        if (!realNodes.empty() || !realWays.empty())
        {
            auto relation_child = o.append_child("relation");
            relation_child.append_attribute("id").set_value(rel.id);


            for (size_t j = 0; j < realNodes.size(); j++)
            {
                auto member_child = relation_child.append_child("member");
                member_child.append_attribute("type").set_value("node");
                member_child.append_attribute("ref").set_value(realNodes[j]);
                if (strlen(realNodeRoles[j]))
                    member_child.append_attribute("role").set_value(realNodeRoles[j]);
            }

            for (size_t j = 0; j < realWays.size(); j++)
            {
                auto member_child = relation_child.append_child("member");
                member_child.append_attribute("type").set_value("way");
                member_child.append_attribute("ref").set_value(realWays[j]);
                if (strlen(realWayRoles[j]))
                    member_child.append_attribute("role").set_value(realWayRoles[j]);
            }

            for (const auto& kv : rel.attrs)
            {
                auto tag_child = relation_child.append_child("tag");
                tag_child.append_attribute("k").set_value(kv.first.c_str());
                tag_child.append_attribute("v").set_value(kv.second.c_str());
                //                        {"v", pfxml::file::decode(kv.second)}};
            }
        }
    }
}


void osm_builder::read_write_ways(pugi::xml_document& i,
                                  pugi::xml_node& o,
                                  osmid_list& ways,
                                  const attribute_key_set& keepAttrs) const
{
    node_id_multimap empty;
    for (const auto& wayxml : i.child("osm").children())
    {
        bool usable = false;
        osm_way w;
        w.id = wayxml.attribute("id").as_ullong();
        for (auto wid : ways)
        {
            if (w.id == wid)
            {
                usable = true;
                break;
            }
        }

        if (usable)
        {
            for (const auto& node : wayxml.children("nd"))
            {
                osmid nid = node.attribute("ref").as_ullong();
                w.nodes.push_back(nid);
            }
            for (const auto& tag : wayxml.children("tag"))
            {
                const auto& key = tag.attribute("k").as_string();
                const auto& val = tag.attribute("v").as_string();
                if (keepAttrs.count(key))
                    w.attrs[key] = val;
            }

            auto way_child = o.append_child("way");
            way_child.append_attribute("id").set_value(w.id);
            for (osmid nid : w.nodes)
            {
                auto nd_child = way_child.append_child("nd");
                nd_child.append_attribute("ref").set_value(nid);
            }
            for (const auto& kv : w.attrs)
            {
                auto tag_child = way_child.append_child("tag");
                tag_child.append_attribute("k").set_value(kv.first.c_str());
                tag_child.append_attribute("v").set_value(kv.second.c_str());
            }
        }
    }
}


node_payload osm_builder::payload_from_gtfs(const gtfs::stop* s, const osm_read_options& ops)
{
    node_payload ret(
            util::geo::latLngToWebMerc(s->stop_lat, s->stop_lon),
            station_info(
                    ops.statNormzer.norm(s->stop_name),
                    ops.trackNormzer.norm(s->platform_code),
                    false));

#ifdef PFAEDLE_STATION_IDS
    // debug feature, store station id from GTFS
    ret.getSI()->setId(s->getId());
#endif

    if (s->get_parent_station().has_value())
    {
        ret.get_si()->add_alternative_name(ops.statNormzer.norm(s->get_parent_station()->get().stop_name));
    }

    return ret;
}


int osm_builder::filter_nodes(pugi::xml_document& xml,
                              osm_id_set& nodes,
                              osm_id_set& nohupNodes,
                              const osm_filter& filter,
                              const bounding_box& bbox) const
{

    for (const auto& node : xml.child("osm").children("node"))
    {
        osmid cur_id = 0;

        double y = node.attribute("lat").as_double();
        double x = node.attribute("lon").as_double();

        if (bbox.contains(Point(x, y)))
        {
            cur_id = util::atoul(node.attribute("id").value());
            nodes.add(cur_id);
        }

        if (cur_id == 0)
            continue;

        for (const auto& tag : node.children("tag"))
        {
            if (filter.nohup(tag.attribute("k").as_string(),
                             tag.attribute("v").as_string()))
            {
                nohupNodes.add(cur_id);
            }
        }
    }

    return 0;
}


bool osm_builder::should_keep_relation(osmid id, const relation_map& rels, const flat_relations& fl) const
{
    auto it = rels.find(id);

    if (it == rels.end()) return false;

    for (osmid rel_id : it->second)
    {
        // as soon as any of this entities relations is not flat, return true
        if (!fl.count(rel_id)) return true;
    }

    return false;
}


bool osm_builder::keep_way(const osm_way& w, const relation_map& wayRels,
                           const osm_filter& filter, const osm_id_set& bBoxNodes,
                           const flat_relations& fl) const
{
    if (w.id && w.nodes.size() > 1 &&
        (should_keep_relation(w.id, wayRels, fl) || filter.keep(w.attrs, osm_filter::WAY)) &&
        !filter.drop(w.attrs, osm_filter::WAY))
    {
        for (osmid nid : w.nodes)
        {
            if (bBoxNodes.has(nid))
            {
                return true;
            }
        }
    }

    return false;
}


void osm_builder::read_ways(pugi::xml_document& xml,
                            const relation_map& wayRels,
                            const osm_filter& filter,
                            const osm_id_set& bBoxNodes,
                            const attribute_key_set& keepAttrs,
                            osmid_list& ret,
                            node_id_map& nodes,
                            const flat_relations& flatRels)
{
    for (const auto& wayxml : xml.child("osm").children("way"))
    {
        osm_way w;
        w.id = wayxml.attribute("id").as_ullong();
        for (const auto& node : wayxml.children("nd"))
        {
            osmid nid = node.attribute("ref").as_ullong();
            w.nodes.push_back(nid);
        }
        for (const auto& tag : wayxml.children("tag"))
        {
            const auto& key = tag.attribute("k").as_string();
            const auto& val = tag.attribute("v").as_string();
            if (keepAttrs.count(key))
                w.attrs[key] = val;
        }

        if (keep_way(w, wayRels, filter, bBoxNodes, flatRels))
        {
            ret.push_back(w.id);
            for (auto n : w.nodes)
            {
                nodes[n] = nullptr;
            }
        }
    }
}


void osm_builder::read_edges(pugi::xml_document& xml,
                             graph& g,
                             const relation_list& rels,
                             const relation_map& wayRels,
                             const osm_filter& filter,
                             const osm_id_set& bBoxNodes,
                             node_id_map& nodes,
                             node_id_multimap& multNodes,
                             const osm_id_set& noHupNodes,
                             const attribute_key_set& keepAttrs,
                             const restrictions& rest,
                             trgraph::restrictor& restor,
                             const flat_relations& flatRels,
                             edge_tracks& etracks,
                             const osm_read_options& opts)
{
    for (const auto& xmlway : xml.child("osm").children("way"))
    {
        osm_way w;
        w.id = xmlway.attribute("id").as_ullong();
        for (const auto& nd : xmlway.children("nd"))
        {
            osmid node_id = nd.attribute("ref").as_ullong();
            w.nodes.emplace_back(node_id);
        }

        for (const auto& tag : xmlway.children("tag"))
        {
            const auto& key = tag.attribute("k").as_string();
            const auto& val = tag.attribute("v").as_string();
            if (keepAttrs.count(key))
            {
                w.attrs[key] = val;
            }
        }
        if (keep_way(w, wayRels, filter, bBoxNodes, flatRels))
        {
            node* last = nullptr;
            std::vector<transit_edge_line*> lines;
            if (wayRels.count(w.id))
            {
                lines = get_lines(wayRels.find(w.id)->second, rels, opts);
            }
            std::string track =
                    getAttrByFirstMatch(opts.edgePlatformRules,
                                        w.id,
                                        w.attrs,
                                        wayRels,
                                        rels,
                                        opts.trackNormzer);

            osmid lastnid = 0;
            for (osmid nid : w.nodes)
            {
                node* n = nullptr;
                if (noHupNodes.has(nid))
                {
                    n = g.addNd();
                    multNodes[nid].insert(n);
                }
                else if (!nodes.count(nid))
                {
                    if (!bBoxNodes.has(nid)) continue;
                    n = g.addNd();
                    nodes[nid] = n;
                }
                else
                {
                    n = nodes[nid];
                }
                if (last)
                {
                    auto e = g.addEdg(last, n, edge_payload());
                    if (!e) continue;

                    process_restrictions(nid, w.id, rest, e, n, restor);
                    process_restrictions(lastnid, w.id, rest, e, last, restor);

                    e->pl().add_lines(lines);
                    e->pl().set_level(filter.level(w.attrs));
                    e->pl().set_max_speed(string_to_kmh(w.attrs["maxspeed"]));
                    if (!track.empty()) etracks[e] = track;

                    if (filter.oneway(w.attrs)) e->pl().setOneWay(1);
                    if (filter.onewayrev(w.attrs)) e->pl().setOneWay(2);
                }
                lastnid = nid;
                last = n;
            }
        }
    }
}


void osm_builder::process_restrictions(osmid nid,
                                       osmid wid,
                                       const restrictions& rawRests,
                                       edge* e,
                                       node* n,
                                       trgraph::restrictor& restor) const
{
    if (rawRests.pos.count(nid))
    {
        for (const auto& kv : rawRests.pos.find(nid)->second)
        {
            if (kv.eFrom == wid)
            {
                e->pl().set_restricted();
                restor.add(e, kv.eTo, n, true);
            }
            else if (kv.eTo == wid)
            {
                e->pl().set_restricted();
                restor.relax(wid, n, e);
            }
        }
    }

    if (rawRests.neg.count(nid))
    {
        for (const auto& kv : rawRests.neg.find(nid)->second)
        {
            if (kv.eFrom == wid)
            {
                e->pl().set_restricted();
                restor.add(e, kv.eTo, n, false);
            }
            else if (kv.eTo == wid)
            {
                e->pl().set_restricted();
                restor.relax(wid, n, e);
            }
        }
    }
}


bool osm_builder::keep_node(const osm_node& n,
                            const node_id_map& nodes,
                            const node_id_multimap& multNodes,
                            const relation_map& nodeRels,
                            const osm_id_set& bBoxNodes,
                            const osm_filter& filter,
                            const flat_relations& fl) const
{
    if (n.id &&
        (nodes.count(n.id) || multNodes.count(n.id) || should_keep_relation(n.id, nodeRels, fl) || filter.keep(n.attrs, osm_filter::NODE)) &&
        (nodes.count(n.id) || bBoxNodes.has(n.id)) &&
        (nodes.count(n.id) || multNodes.count(n.id) || !filter.drop(n.attrs, osm_filter::NODE)))
    {
        return true;
    }

    return false;
}


void osm_builder::read_write_nodes(pugi::xml_document& i,
                                   pugi::xml_node& o,
                                   const relation_map& nRels,
                                   const osm_filter& filter,
                                   const osm_id_set& bBoxNds,
                                   node_id_map& nds,
                                   const attribute_key_set& keepAttrs,
                                   const flat_relations& f) const
{
    node_id_multimap empt;
    for (const auto& xmlnode : i.child("osm").children("node"))
    {
        osm_node nd;
        nd.lat = xmlnode.attribute("lat").as_double();
        nd.lng = xmlnode.attribute("lon").as_double();
        nd.id = xmlnode.attribute("id").as_ullong();

        for (const auto& tag : xmlnode.children("tag"))
        {
            const auto& key = tag.attribute("k").as_string();
            const auto& val = tag.attribute("v").as_string();

            if (keepAttrs.count(key))
                nd.attrs[key] = val;
        }

        if (keep_node(nd, nds, empt, nRels, bBoxNds, filter, f))
        {
            nds[nd.id] = nullptr;
            pugi::xml_node node = o.append_child("node");
            node.append_attribute("id").set_value(nd.id);
            node.append_attribute("lat").set_value(nd.lat);
            node.append_attribute("lon").set_value(nd.lng);
            for (const auto& kv : nd.attrs)
            {
                pugi::xml_node tag = node.append_child("tag");
                tag.append_attribute("k").set_value(kv.first.c_str());
                tag.append_attribute("v").set_value(kv.second.c_str());
            }
        }
    }
}


void osm_builder::read_nodes(pugi::xml_document& xml,
                             graph& g,
                             const relation_list& rels,
                             const relation_map& nodeRels,
                             const osm_filter& filter,
                             const osm_id_set& bBoxNodes,
                             node_id_map& nodes,
                             node_id_multimap& multNodes,
                             router::node_set& orphanStations,
                             const attribute_key_set& keepAttrs,
                             const flat_relations& fl,
                             const osm_read_options& opts) const
{
    station_attribute_groups attr_groups;

    for (const auto& xmlnode : xml.child("osm").children("node"))
    {
        osm_node nd;
        nd.lat = xmlnode.attribute("lat").as_double();
        nd.lng = xmlnode.attribute("lon").as_double();
        nd.id = xmlnode.attribute("id").as_ullong();

        for (const auto& tag : xmlnode.children("tag"))
        {
            const auto& key = tag.attribute("k").as_string();
            const auto& val = tag.attribute("v").as_string();

            if (keepAttrs.count(key))
                nd.attrs[key] = val;
        }

        if (keep_node(nd, nodes, multNodes, nodeRels, bBoxNodes, filter, fl))
        {
            node* n = nullptr;
            auto pos = util::geo::latLngToWebMerc(nd.lat, nd.lng);
            if (nodes.count(nd.id))
            {
                n = nodes[nd.id];
                n->pl().set_geom(pos);
                if (filter.station(nd.attrs))
                {
                    auto si = get_station_info(n, nd.id, pos, nd.attrs, &attr_groups, nodeRels,
                                               rels, opts);
                    if (si.has_value())
                    {
                        n->pl().set_si(si.value());
                    }
                }
                else if (filter.blocker(nd.attrs))
                {
                    n->pl().set_blocker();
                }
            }
            else if (multNodes.count(nd.id))
            {
                for (auto* node : multNodes[nd.id])
                {
                    node->pl().set_geom(pos);
                    if (filter.station(nd.attrs))
                    {
                        auto si = get_station_info(node, nd.id, pos, nd.attrs, &attr_groups, nodeRels,
                                                   rels, opts);
                        if (si.has_value())
                        {
                            node->pl().set_si(si.value());
                        }
                    }
                    else if (filter.blocker(nd.attrs))
                    {
                        node->pl().set_blocker();
                    }
                }
            }
            else
            {
                // these are nodes without any connected edges
                if (filter.station(nd.attrs))
                {
                    auto tmp = g.addNd(node_payload(pos));
                    auto si = get_station_info(tmp, nd.id, pos, nd.attrs, &attr_groups, nodeRels, rels, opts);

                    if (si.has_value())
                    {
                        tmp->pl().set_si(si.value());
                    }

                    if (tmp->pl().get_si())
                    {
                        tmp->pl().get_si()->set_is_from_osm(false);
                        orphanStations.insert(tmp);
                    }
                }
            }
        }
    }
}


void osm_builder::read_relations(pugi::xml_document& xml,
                                 relation_list& rels,
                                 relation_map& nodeRels,
                                 relation_map& wayRels,
                                 const osm_filter& filter,
                                 const attribute_key_set& keepAttrs,
                                 restrictions& rests) const
{
    for (const auto& xmlrel : xml.child("osm").children("relation"))
    {
        osm_relation rel;
        uint64_t keep_flags = 0;
        uint64_t drop_flags = 0;

        rel.id = xmlrel.attribute("id").as_ullong();
        // processing attributes
        {
            for (const auto& tag : xmlrel.children("tag"))
            {
                const auto& key = tag.attribute("k").as_string();
                const auto& value = tag.attribute("v").as_string();
                if (keepAttrs.count(key))
                {
                    rel.attrs[key] = value;
                }
            }

            if (rel.id && !rel.attrs.empty() &&
                (keep_flags = filter.keep(rel.attrs, osm_filter::REL)) &&
                !(drop_flags = filter.drop(rel.attrs, osm_filter::REL)))
            {
                rel.keepFlags = keep_flags;
                rel.dropFlags = drop_flags;
            }

            rels.rels.emplace_back(rel.attrs);
        }

        // processing members
        for (const auto& member : xmlrel.children("member"))
        {
            const auto& type = member.attribute("type").as_string();
            if (strcmp(type, "node") == 0)
            {
                osmid id = member.attribute("ref").as_ullong();
                rel.nodes.emplace_back(id);
                rel.nodeRoles.emplace_back(member.attribute("role").as_string());
            }
            if (strcmp(type, "way") == 0)
            {
                osmid id = member.attribute("ref").as_ullong();
                rel.ways.emplace_back(id);
                rel.wayRoles.emplace_back(member.attribute("role").as_string());
            }
        }

        if (rel.keepFlags & osm::REL_NO_DOWN)
        {
            rels.flat.insert(rels.rels.size() - 1);
        }
        for (osmid id : rel.nodes)
        {
            nodeRels[id].push_back(rels.rels.size() - 1);
        }
        for (osmid id : rel.ways)
        {
            wayRels[id].push_back(rels.rels.size() - 1);
        }

        // TODO(patrick): this is not needed for the filtering - remove it here!
        read_restrictions(rel, rests, filter);
    }
}


void osm_builder::read_restrictions(const osm_relation& rel,
                                    restrictions& rests,
                                    const osm_filter& filter) const
{
    if (!rel.attrs.count("type") || rel.attrs.find("type")->second != "restriction")
        return;

    bool pos = filter.posRestr(rel.attrs);
    bool neg = filter.negRestr(rel.attrs);

    if (!pos && !neg)
        return;

    osmid from = 0;
    osmid to = 0;
    osmid via = 0;

    for (size_t i = 0; i < rel.ways.size(); i++)
    {
        if (rel.wayRoles[i] == "from")
        {
            if (from) return;// only one from member supported
            from = rel.ways[i];
        }
        if (rel.wayRoles[i] == "to")
        {
            if (to) return;// only one to member supported
            to = rel.ways[i];
        }
    }

    for (size_t i = 0; i < rel.nodes.size(); i++)
    {
        if (rel.nodeRoles[i] == "via")
        {
            via = rel.nodes[i];
            break;
        }
    }

    if (from && to && via)
    {
        if (pos)
            rests.pos[via].emplace_back(from, to);
        else if (neg)
            rests.neg[via].emplace_back(from, to);
    }
}


std::string osm_builder::getAttrByFirstMatch(const deep_attribute_list& rule, osmid id,
                                             const attribute_map& attrs,
                                             const relation_map& entRels,
                                             const relation_list& rels,
                                             const normalizer& normzer) const
{
    std::string ret;
    for (const auto& s : rule)
    {
        ret = normzer.norm(get_attribute(s, id, attrs, entRels, rels));
        if (!ret.empty()) return ret;
    }
    return ret;
}


std::vector<std::string> osm_builder::getAttrMatchRanked(
        const deep_attribute_list& rule, osmid id, const attribute_map& attrs,
        const relation_map& entRels, const relation_list& rels,
        const normalizer& normzer) const
{
    std::vector<std::string> ret;
    for (const auto& s : rule)
    {
        std::string tmp = normzer.norm(get_attribute(s, id, attrs, entRels, rels));
        if (!tmp.empty())
        {
            ret.push_back(tmp);
        }
    }
    return ret;
}


std::string osm_builder::get_attribute(const deep_attribute_rule& s, osmid id,
                                       const attribute_map& attrs, const relation_map& entRels,
                                       const relation_list& rels) const
{
    if (s.relRule.kv.first.empty())
    {
        if (attrs.find(s.attr) != attrs.end())
        {
            return attrs.find(s.attr)->second;
        }
    }
    else
    {
        if (entRels.count(id))
        {
            for (const auto& rel_id : entRels.find(id)->second)
            {
                if (osm_filter::contained(rels.rels[rel_id], s.relRule.kv))
                {
                    if (rels.rels[rel_id].count(s.attr))
                    {
                        return rels.rels[rel_id].find(s.attr)->second;
                    }
                }
            }
        }
    }
    return "";
}


std::optional<station_info> osm_builder::get_station_info(node* node, osmid nid,
                                                          const POINT& pos, const attribute_map& m,
                                                          station_attribute_groups* groups,
                                                          const relation_map& nodeRels,
                                                          const relation_list& rels,
                                                          const osm_read_options& ops) const
{
    std::string platform;
    std::vector<std::string> names;

    names = getAttrMatchRanked(ops.statAttrRules.nameRule, nid, m, nodeRels, rels,
                               ops.statNormzer);
    platform = getAttrByFirstMatch(ops.statAttrRules.platformRule, nid, m,
                                   nodeRels, rels, ops.trackNormzer);

    if (names.empty())
        return std::nullopt;

    auto ret = station_info(names[0], platform, true);

#ifdef PFAEDLE_STATION_IDS
    ret.setId(getAttrByFirstMatch(ops.statAttrRules.idRule, nid, m, node_relations_,
                                  rels, ops.idNormzer));
#endif

    for (size_t i = 1; i < names.size(); i++) ret.add_alternative_name(names[i]);

    bool group_found = false;

    for (const auto& rule : ops.statGroupNAttrRules)
    {
        if (group_found) break;
        std::string rule_val = get_attribute(rule.attr, nid, m, nodeRels, rels);
        if (!rule_val.empty())
        {
            // check if a matching group exists
            for (auto* group : (*groups)[rule.attr.attr][rule_val])
            {
                if (group_found) break;
                for (const auto* member : group->get_nodes())
                {
                    if (webMercMeterDist(*member->pl().get_geom(), pos) <= rule.maxDist)
                    {
                        // ok, group is matching
                        group_found = true;
                        if (node) group->add_node(node);
                        ret.set_group(group);
                        break;
                    }
                }
            }
        }
    }

    if (!group_found)
    {
        for (const auto& rule : ops.statGroupNAttrRules)
        {
            std::string rule_val = get_attribute(rule.attr, nid, m, nodeRels, rels);
            if (!rule_val.empty())
            {
                // add new group
                auto* g = new trgraph::station_group();
                if (node)
                    g->add_node(node);
                ret.set_group(g);
                (*groups)[rule.attr.attr][rule_val].push_back(g);
                break;
            }
        }
    }

    return ret;
}


double osm_builder::webMercDist(const node& a, const node& b)
{
    return webMercMeterDist(*a.pl().get_geom(), *b.pl().get_geom());
}


node* osm_builder::depth_search(const edge* e, const station_info* si, const POINT& p,
                                double maxD, int maxFullTurns, double minAngle,
                                const search_functor& sfunc)
{
    // shortcuts
    double dFrom = webMercMeterDist(*e->getFrom()->pl().get_geom(), p);
    double dTo = webMercMeterDist(*e->getTo()->pl().get_geom(), p);

    if (dFrom > maxD && dTo > maxD)
        return nullptr;

    if (dFrom <= maxD && sfunc(e->getFrom(), si)) return e->getFrom();
    if (dTo <= maxD && sfunc(e->getTo(), si)) return e->getTo();

    node_candidate_priority_queue pq;
    router::node_set closed;
    pq.push(NodeCand{dFrom, e->getFrom(), e, 0});
    if (e->getFrom() != e->getTo()) pq.push(NodeCand{dTo, e->getTo(), e, 0});

    while (!pq.empty())
    {
        auto cur = pq.top();
        pq.pop();
        if (closed.count(cur.node)) continue;
        closed.insert(cur.node);

        for (size_t i = 0; i < cur.node->getInDeg() + cur.node->getOutDeg(); i++)
        {
            trgraph::node* cand;
            trgraph::edge* edg;

            if (i < cur.node->getInDeg())
            {
                edg = cur.node->getAdjListIn()[i];
                cand = edg->getFrom();
            }
            else
            {
                edg = cur.node->getAdjListOut()[i - cur.node->getInDeg()];
                cand = edg->getTo();
            }

            if (cand == cur.node) continue;// dont follow self edges

            int fullTurn = 0;

            if (cur.fromEdge && cur.node->getInDeg() + cur.node->getOutDeg() >
                                        2)
            {// only intersection angles
                const POINT& toP = *cand->pl().get_geom();
                const POINT& fromP =
                        *cur.fromEdge->getOtherNd(cur.node)->pl().get_geom();
                const POINT& nodeP = *cur.node->pl().get_geom();

                if (util::geo::innerProd(nodeP, fromP, toP) < minAngle) fullTurn = 1;
            }

            if ((maxFullTurns < 0 || cur.fullTurns + fullTurn <= maxFullTurns) &&
                cur.dist + edg->pl().get_length() < maxD && !closed.count(cand))
            {
                if (sfunc(cand, si))
                {
                    return cand;
                }
                else
                {
                    pq.push(NodeCand{cur.dist + edg->pl().get_length(), cand, edg,
                                     cur.fullTurns + fullTurn});
                }
            }
        }
    }

    return nullptr;
}


bool osm_builder::is_blocked(const edge* e,
                             const station_info* si,
                             const POINT& p,
                             double maxD,
                             int maxFullTurns,
                             double minAngle)
{
    return depth_search(e, si, p, maxD, maxFullTurns, minAngle, block_search_functor());
}


node* osm_builder::eqStatReach(const edge* e, const station_info* si, const POINT& p,
                               double maxD, int maxFullTurns, double minAngle,
                               bool orphanSnap)
{
    return depth_search(e, si, p, maxD, maxFullTurns, minAngle, eq_search_functor(orphanSnap));
}


std::set<node*> osm_builder::snap_station(graph& g,
                                          node_payload& s,
                                          trgraph::edge_grid& eg,
                                          trgraph::node_grid& sng,
                                          const osm_read_options& opts,
                                          trgraph::restrictor& restor,
                                          bool surHeur,
                                          bool orphSnap,
                                          double maxD,
                                          bool import_osm_stations)
{
    assert(s.get_si());
    std::set<node*> ret;

    edge_candidate_priority_queue pq;

    if (import_osm_stations)
    {
        // importing data from osm -> mainly used to correct stations and positions
        const auto* best = sng.get_distance_matching_node(s, opts.maxSnapFallbackHeurDistance);
        if (best)
        {
            pq = eg.get_edge_candidates(*best->pl().get_geom(), maxD);
            s.set_si(station_info(*best->pl().get_si()));
        }
        else
        {
            pq = eg.get_edge_candidates(*s.get_geom(), maxD);
        }
    }
    else
    {
        // fallback to normal operating mode
        pq = eg.get_edge_candidates(*s.get_geom(), maxD);
    }

    if (pq.empty() && surHeur)
    {
        // no station found in the first round, try again with the nearest
        // surrounding station with matching name
        const node* best = sng.get_matching_node(s, opts.maxSnapFallbackHeurDistance);
        if (best)
        {
            pq = eg.get_edge_candidates(*best->pl().get_geom(), maxD);
        }
        else
        {
            // if still no luck, get edge cands in fallback snap distance
            pq = eg.get_edge_candidates(*s.get_geom(), opts.maxSnapFallbackHeurDistance);
        }
    }

    while (!pq.empty())
    {
        auto* e = pq.top().second;
        pq.pop();
        auto geom = util::geo::projectOn(*e->getFrom()->pl().get_geom(),
                                         *s.get_geom(),
                                         *e->getTo()->pl().get_geom());

        node* eq = nullptr;
        if (!(eq = eqStatReach(e, s.get_si(), geom, 2 * maxD, 0, opts.maxAngleSnapReach, orphSnap)))
        {
            if (e->pl().level() > opts.maxSnapLevel)
                continue;
            if (is_blocked(e,
                           s.get_si(),
                           geom,
                           opts.maxBlockDistance,
                           0,
                           opts.maxAngleSnapReach))
            {
                continue;
            }

            // if the projected position is near (< 2 meters) the end point of this
            // way and the endpoint is not already a station, place the station there.
            if (!e->getFrom()->pl().get_si() && webMercMeterDist(geom, *e->getFrom()->pl().get_geom()) < 2)
            {
                e->getFrom()->pl().set_si(*s.get_si());

                if (s.get_si()->get_group())
                    s.get_si()->get_group()->add_node(e->getFrom());

                ret.insert(e->getFrom());
            }
            else if (!e->getTo()->pl().get_si() && webMercMeterDist(geom, *e->getTo()->pl().get_geom()) < 2)
            {
                e->getTo()->pl().set_si(*s.get_si());

                if (s.get_si()->get_group())
                    s.get_si()->get_group()->add_node(e->getTo());

                ret.insert(e->getTo());
            }
            else
            {
                s.set_geom(geom);
                node* n = g.addNd(s);

                if (n->pl().get_si()->get_group())
                    n->pl().get_si()->get_group()->add_node(n);

                sng.add(geom, n);

                auto ne = g.addEdg(e->getFrom(), n, e->pl());
                ne->pl().set_length(webMercDist(*n, *e->getFrom()));
                LINE l;
                l.push_back(*e->getFrom()->pl().get_geom());
                l.push_back(*n->pl().get_geom());
                *ne->pl().get_geom() = l;
                eg.add(l, ne);

                auto nf = g.addEdg(n, e->getTo(), e->pl());
                nf->pl().set_length(webMercDist(*n, *e->getTo()));
                LINE ll;
                ll.push_back(*n->pl().get_geom());
                ll.push_back(*e->getTo()->pl().get_geom());
                *nf->pl().get_geom() = ll;
                eg.add(ll, nf);

                // replace edge in restrictor
                restor.replace_edge(e, ne, nf);

                g.delEdg(e->getFrom(), e->getTo());
                eg.remove(e);
                ret.insert(n);
            }
        }
        else
        {
            // if the snapped station is very near to the original OSM station
            // write additional info from this snap station to the equivalent stat
            if (webMercMeterDist(*s.get_geom(), *eq->pl().get_geom()) < opts.maxOsmStationDistance)
            {
                if (eq->pl().get_si()->get_track().empty())
                    eq->pl().get_si()->set_track(s.get_si()->get_track());
            }
            ret.insert(eq);
        }
    }

    return ret;
}


trgraph::station_group* osm_builder::group_stats(const router::node_set& s)
{
    if (s.empty())
        return nullptr;
    // reference group
    auto* ret = new trgraph::station_group();
    bool used = false;

    for (auto* n : s)
    {
        if (!n->pl().get_si())
            continue;
        used = true;
        if (n->pl().get_si()->get_group())
        {
            // this node is already in a group - merge this group with this one
            ret->merge(n->pl().get_si()->get_group());
        }
        else
        {
            ret->add_node(n);
            n->pl().get_si()->set_group(ret);
        }
    }

    if (!used)
    {
        delete ret;
        return nullptr;
    }

    return ret;
}


std::vector<transit_edge_line*> osm_builder::get_lines(
        const std::vector<size_t>& edgeRels,
        const relation_list& rels,
        const osm_read_options& ops)
{
    std::vector<transit_edge_line*> ret;
    for (size_t rel_id : edgeRels)
    {
        transit_edge_line* elp = nullptr;

        if (_relLines.count(rel_id))
        {
            elp = _relLines[rel_id];
        }
        else
        {
            transit_edge_line el;

            bool found = false;
            for (const auto& r : ops.relLinerules.sNameRule)
            {
                for (const auto& rel_attr : rels.rels[rel_id])
                {
                    if (rel_attr.first == r)
                    {
                        el.shortName = ops.lineNormzer.norm(rel_attr.second);
                        //ops.lineNormzer.norm(pfxml::file::decode(relAttr.second));
                        if (!el.shortName.empty())
                            found = true;
                    }
                }
                if (found) break;
            }

            found = false;
            for (const auto& r : ops.relLinerules.fromNameRule)
            {
                for (const auto& rel_attr : rels.rels[rel_id])
                {
                    if (rel_attr.first == r)
                    {
                        el.fromStr = ops.lineNormzer.norm(rel_attr.second);
                        //ops.statNormzer.norm(pfxml::file::decode(relAttr.second));
                        if (!el.fromStr.empty())
                            found = true;
                    }
                }
                if (found)
                    break;
            }

            found = false;
            for (const auto& r : ops.relLinerules.toNameRule)
            {
                for (const auto& rel_attr : rels.rels[rel_id])
                {
                    if (rel_attr.first == r)
                    {
                        el.toStr = ops.lineNormzer.norm(rel_attr.second);
                        //ops.statNormzer.norm(pfxml::file::decode(relAttr.second));
                        if (!el.toStr.empty()) found = true;
                    }
                }
                if (found) break;
            }

            if (el.shortName.empty() && el.fromStr.empty() && el.toStr.empty())
                continue;

            if (_lines.count(el))
            {
                elp = _lines[el];
                _relLines[rel_id] = elp;
            }
            else
            {
                elp = new transit_edge_line(el);
                _lines[el] = elp;
                _relLines[rel_id] = elp;
            }
        }
        ret.push_back(elp);
    }
    return ret;
}


void osm_builder::write_edge_tracks(const edge_tracks& tracks)
{
    for (const auto& tr : tracks)
    {
        if (tr.first->getTo()->pl().get_si() &&
            tr.first->getTo()->pl().get_si()->get_track().empty())
        {
            tr.first->getTo()->pl().get_si()->set_track(tr.second);
        }
        if (tr.first->getFrom()->pl().get_si() &&
            tr.first->getFrom()->pl().get_si()->get_track().empty())
        {
            tr.first->getFrom()->pl().get_si()->set_track(tr.second);
        }
    }
}


void osm_builder::snap_stations(const osm_read_options& opts,
                                graph& g,
                                const bounding_box& bbox,
                                size_t gridSize,
                                router::feed_stops& fs,
                                trgraph::restrictor& res,
                                const router::node_set& orphanStations,
                                const bool import_osm_stations)
{
    trgraph::node_grid sng = trgraph::node_grid::build_node_grid(g, gridSize, bbox.get_full_web_merc_box(), true);
    trgraph::edge_grid eg = trgraph::edge_grid::build_edge_grid(g, gridSize, bbox.get_full_web_merc_box());

    LOG(DEBUG) << "Grid size of " << sng.getXWidth() << "x" << sng.getYHeight();

    for (double d : opts.maxSnapDistances)
    {
        for (auto s : orphanStations)
        {
            POINT geom = *s->pl().get_geom();
            node_payload pl = s->pl();
            pl.get_si()->set_is_from_osm(false);
            const auto& r = snap_station(g, pl, eg, sng, opts, res, false, false, d, import_osm_stations);
            group_stats(r);
            for (auto n : r)
            {
                // if the snapped station is very near to the original OSM
                // station, set is-from-osm to true
                if (webMercMeterDist(geom, *n->pl().get_geom()) < opts.maxOsmStationDistance)
                {
                    if (n->pl().get_si()) n->pl().get_si()->set_is_from_osm(true);
                }
            }
        }
    }

    std::vector<const gtfs::stop*> not_snapped;

    for (auto& s : fs)
    {
        bool snapped = false;
        auto pl = payload_from_gtfs(s.first, opts);
        for (size_t i = 0; i < opts.maxSnapDistances.size(); i++)
        {
            double d = opts.maxSnapDistances[i];

            trgraph::station_group* group = group_stats(
                    snap_station(g, pl, eg, sng, opts, res,
                                 i == opts.maxSnapDistances.size() - 1, false, d, import_osm_stations));

            if (group)
            {
                group->add_stop(s.first);
                fs[s.first] = *group->get_nodes().begin();
                snapped = true;
            }
        }
        if (!snapped)
        {
            LOG(TRACE) << "Could not snap station "
                       << "(" << pl.get_si()->get_name() << ")"
                       << " (" << s.first->stop_lon << "," << s.first->stop_lon
                       << ") in normal run, trying again later in orphan mode.";
            if (!bbox.contains(*pl.get_geom()))
            {
                LOG(TRACE) << "Note: '" << pl.get_si()->get_name()
                           << "' does not lie within the bounds for this graph and "
                              "may be a stray station";
            }
            not_snapped.push_back(s.first);
        }
    }

    if (!not_snapped.empty())
        LOG(TRACE) << not_snapped.size()
                   << " stations could not be snapped in "
                      "normal run, trying again in orphan "
                      "mode.";

    // try again, but aggressively snap to orphan OSM stations which have
    // not been assigned to any GTFS stop yet
    for (auto& s : not_snapped)
    {
        bool snapped = false;
        auto pl = payload_from_gtfs(s, opts);
        for (size_t i = 0; i < opts.maxSnapDistances.size(); i++)
        {
            double d = opts.maxSnapDistances[i];

            trgraph::station_group* group = group_stats(
                    snap_station(g, pl, eg, sng, opts, res,
                                 i == opts.maxSnapDistances.size() - 1, true, d, import_osm_stations));

            if (group)
            {
                group->add_stop(s);
                // add the added station name as an alt name to ensure future
                // similarity
                for (auto n : group->get_nodes())
                {
                    if (n->pl().get_si())
                        n->pl().get_si()->add_alternative_name(pl.get_si()->get_name());
                }
                fs[s] = *group->get_nodes().begin();
                snapped = true;
            }
        }
        if (!snapped)
        {
            // finally give up

            // add a group with only this stop in it
            auto* dummy_group = new trgraph::station_group();
            auto* dummy_node = g.addNd(pl);

            dummy_node->pl().get_si()->set_group(dummy_group);
            dummy_group->add_node(dummy_node);
            dummy_group->add_stop(s);
            fs[s] = dummy_node;
            if (!bbox.contains(*pl.get_geom()))
            {
                LOG(TRACE) << "Could not snap station "
                           << "(" << pl.get_si()->get_name() << ")"
                           << " (" << s->stop_lat << "," << s->stop_lon << ")";
                LOG(TRACE) << "Note: '" << pl.get_si()->get_name()
                           << "' does not lie within the bounds for this graph and "
                              "may be a stray station";
            }
            else
            {
                // only warn if it is contained in the BBOX for this graph
                LOG(WARN) << "Could not snap station "
                          << "(" << pl.get_si()->get_name() << ")"
                          << " (" << s->stop_lat << "," << s->stop_lon << ")";
            }
        }
    }
}
}
