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

//#include <osmium/handler/node_locations_for_ways.hpp>
#include "osm_reader.h"

#include <logging/logger.h>
#include <pugixml.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <fstream>

using util::geo::webMercMeterDist;
using util::geo::Point;

namespace pfaedle::osm
{

bool eq_search_functor::operator()(const trgraph::node* cand, const trgraph::station_info* si) const
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
                       trgraph::graph& g,
                       const bounding_box& bbox,
                       size_t gridSize,
                       router::feed_stops& fs,
                       trgraph::restrictor& res,
                       bool import_osm_stations)
{
    if (!bbox.size())
        return;

    LOG(INFO) << "Reading OSM file " << path << " ... ";
    osm_reader::read_configuration read_configuration { opts,g,res};
    LOG_INFO()<<"Start reading pbf";
    osm_reader reader{read_configuration};
    reader.read(path, bbox);
    LOG_INFO()<<"Finish reading pbf";

    {
        LOG(TRACE) << "Fixing gaps...";
        trgraph::node_grid ng = trgraph::node_grid::build_node_grid(g, gridSize, bbox.get_full_web_merc_box(), false);
        g.fix_gaps(ng);
    }

    LOG(TRACE) << "Writing edge geoms...";
    g.write_geometries();

    LOG(TRACE) << "Snapping stations...";
    snap_stations(opts, g, bbox, gridSize, fs, res, reader.get_orphan_stations(), import_osm_stations);

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


void osm_builder::filter_write(const std::string& input_file,
                               const std::string& output_file,
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
    input_doc.load_file(input_file.c_str());

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

    //filter_nodes(input_doc, bboxNodes, noHupNodes, filter, box);

    //read_relations(input_doc, rels, nodeRels, wayRels, filter, attr_keys[2], rests);
    read_ways(input_doc, wayRels, filter, bboxNodes, attr_keys[1], ways, nodes, rels.flat);

    read_write_nodes(input_doc, osm_child, nodeRels, filter, bboxNodes, nodes, attr_keys[0], rels.flat);
    read_write_ways(input_doc, osm_child, ways, attr_keys[1]);

    std::sort(ways.begin(), ways.end());
    read_write_relations(input_doc, osm_child, ways, nodes, filter, attr_keys[2]);

    std::ofstream outstr{output_file};
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


trgraph::node_payload osm_builder::payload_from_gtfs(const gtfs::stop* s, const osm_read_options& ops)
{
    trgraph::node_payload ret(
            util::geo::latLngToWebMerc(s->stop_lat, s->stop_lon),
            trgraph::station_info(
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





void osm_builder::read_ways(const pugi::xml_document& xml,
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

        if (w.keep_way(wayRels, filter, bBoxNodes, flatRels))
        {
            ret.push_back(w.id);
            for (auto n : w.nodes)
            {
                nodes[n] = nullptr;
            }
        }
    }
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

        if (nd.keep_node(nds, empt, nRels, bBoxNds, filter, f))
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



double osm_builder::webMercDist(const trgraph::node& a, const trgraph::node& b)
{
    return webMercMeterDist(*a.pl().get_geom(), *b.pl().get_geom());
}


trgraph::node* osm_builder::depth_search(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
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


bool osm_builder::is_blocked(const trgraph::edge* e,
                             const trgraph::station_info* si,
                             const POINT& p,
                             double maxD,
                             int maxFullTurns,
                             double minAngle)
{
    return depth_search(e, si, p, maxD, maxFullTurns, minAngle, block_search_functor());
}


trgraph::node* osm_builder::eqStatReach(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
                               double maxD, int maxFullTurns, double minAngle,
                               bool orphanSnap)
{
    return depth_search(e, si, p, maxD, maxFullTurns, minAngle, eq_search_functor(orphanSnap));
}


std::set<trgraph::node*> osm_builder::snap_station(trgraph::graph& g,
                                                   trgraph::node_payload& s,
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
    std::set<trgraph::node*> ret;

    edge_candidate_priority_queue pq;

    if (import_osm_stations)
    {
        // importing data from osm -> mainly used to correct stations and positions
        const auto* best = sng.get_distance_matching_node(s, opts.maxSnapFallbackHeurDistance);
        if (best)
        {
            pq = eg.get_edge_candidates(*best->pl().get_geom(), maxD);
            s.set_si(trgraph::station_info(*best->pl().get_si()));
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
        const trgraph::node* best = sng.get_matching_node(s, opts.maxSnapFallbackHeurDistance);
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

        trgraph::node* eq = nullptr;
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
                trgraph::node* n = g.addNd(s);

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


void osm_builder::snap_stations(const osm_read_options& opts,
                                trgraph::graph& g,
                                const bounding_box& bbox,
                                size_t gridSize,
                                router::feed_stops& fs,
                                trgraph::restrictor& res,
                                const router::node_set& orphanStations,
                                bool import_osm_stations)
{
    trgraph::node_grid sng = trgraph::node_grid::build_node_grid(g, gridSize, bbox.get_full_web_merc_box(), true);
    trgraph::edge_grid eg = trgraph::edge_grid::build_edge_grid(g, gridSize, bbox.get_full_web_merc_box());

    LOG(DEBUG) << "Grid size of " << sng.getXWidth() << "x" << sng.getYHeight();

    for (double d : opts.maxSnapDistances)
    {
        for (auto s : orphanStations)
        {
            POINT geom = *s->pl().get_geom();
            trgraph::node_payload pl = s->pl();
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
