// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/osm/OsmBuilder.h"
#include "pfaedle/Def.h"
#include "pfaedle/osm/BBoxIdx.h"
#include "pfaedle/osm/Osm.h"
#include "pfaedle/osm/OsmFilter.h"
#include "pfaedle/osm/Restrictor.h"
#include "pfaedle/trgraph/StatGroup.h"
#include "util/Misc.h"
#include "util/Nullable.h"

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

using ad::cppgtfs::gtfs::Stop;
using pfaedle::osm::BlockSearch;
using pfaedle::osm::EdgeGrid;
using pfaedle::osm::EqSearch;
using pfaedle::osm::NodeGrid;
using pfaedle::osm::OsmBuilder;
using pfaedle::osm::OsmNode;
using pfaedle::osm::OsmRel;
using pfaedle::osm::OsmWay;
using pfaedle::trgraph::Component;
using pfaedle::trgraph::Edge;
using pfaedle::trgraph::EdgePayload;
using pfaedle::trgraph::Graph;
using pfaedle::trgraph::Node;
using pfaedle::trgraph::NodePayload;
using pfaedle::trgraph::Normalizer;
using pfaedle::trgraph::StatGroup;
using pfaedle::trgraph::StatInfo;
using pfaedle::trgraph::TransitEdgeLine;
using util::Nullable;
using util::geo::webMercMeterDist;


bool EqSearch::operator()(const Node* cand, const StatInfo* si) const
{
    if (orphanSnap && cand->pl().getSI() &&
        (!cand->pl().getSI()->getGroup() ||
         cand->pl().getSI()->getGroup()->getStops().empty()))
    {
        return true;
    }
    return cand->pl().getSI() && cand->pl().getSI()->simi(si) > minSimi;
}


OsmBuilder::OsmBuilder() = default;


void OsmBuilder::read(const std::string& path,
                      const OsmReadOpts& opts,
                      Graph& g,
                      const BBoxIdx& bbox,
                      size_t gridSize,
                      router::FeedStops& fs,
                      Restrictor& res)
{
    if (!bbox.size()) return;

    LOG(INFO) << "Reading OSM file " << path << " ... ";

    NodeSet orphan_stations;
    EdgTracks e_tracks;
    {
        OsmIdSet bboxNodes, noHupNodes;

        NIdMap nodes;
        NIdMultMap mult_nodes;
        RelLst intm_rels;
        RelMap node_rels, wayRels;

        Restrictions raw_rests;

        AttrKeySet attr_keys[3] = {};
        getKeptAttrKeys(opts, attr_keys);

        OsmFilter filter(opts);

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
        filter_nodes(doc, &bboxNodes, &noHupNodes, filter, bbox);

        LOG(TRACE) << "Reading relations...";
        readRels(doc, intm_rels, node_rels, wayRels, filter, attr_keys[2], raw_rests);

        LOG(TRACE) << "Reading edges...";
        readEdges(doc, g, intm_rels, wayRels, filter, bboxNodes, nodes, mult_nodes,
                  noHupNodes, attr_keys[1], raw_rests, res, intm_rels.flat, e_tracks,
                  opts);

        LOG(TRACE) << "Reading kept nodes...";
        readNodes(doc, g, intm_rels, node_rels, filter, bboxNodes, nodes,
                  mult_nodes, orphan_stations, attr_keys[0], intm_rels.flat, opts);
    }

    LOG(TRACE) << "OSM ID set lookups: " << osm::OsmIdSet::LOOKUPS
                << ", file lookups: " << osm::OsmIdSet::FLOOKUPS;

    LOG(TRACE) << "Applying edge track numbers...";
    writeEdgeTracks(e_tracks);
    e_tracks.clear();

    {
        LOG(TRACE) << "Fixing gaps...";
        NodeGrid ng = buildNodeIdx(g, gridSize, bbox.getFullWebMercBox(), false);
        fixGaps(g, &ng);
    }

    LOG(TRACE) << "Writing edge geoms...";
    writeGeoms(g);

    LOG(TRACE) << "Snapping stations...";
    snapStats(opts, g, bbox, gridSize, fs, res, orphan_stations);

    LOG(TRACE) << "Deleting orphan nodes...";
    deleteOrphNds(g);

    LOG(TRACE) << "Deleting orphan edges...";
    deleteOrphEdgs(g, opts);

    LOG(TRACE) << "Collapsing edges...";
    collapseEdges(g);

    LOG(TRACE) << "Deleting orphan nodes...";
    deleteOrphNds(g);

    LOG(TRACE) << "Deleting orphan edges...";
    deleteOrphEdgs(g, opts);

    LOG(TRACE) << "Writing graph components...";
    // the restrictor is needed here to prevent connections in the graph
    // which are not possible in reality
    uint32_t comps = writeComps(g);

    LOG(TRACE) << "Simplifying geometries...";
    simplifyGeoms(g);

    LOG(TRACE) << "Writing other-direction edges...";
    writeODirEdgs(g, res);

    LOG(TRACE) << "Write dummy node self-edges...";
    writeSelfEdgs(g);

    size_t num_edges = 0;

    for (auto* n : *g.getNds())
    {
        num_edges += n->getAdjListOut().size();
    }

    LOG(DEBUG) << "Graph has " << g.getNds()->size() << " nodes, " << num_edges
               << " edges and " << comps << " connected component(s)";
}


void OsmBuilder::overpassQryWrite(std::ostream* out,
                                  const std::vector<OsmReadOpts>& opts,
                                  const BBoxIdx& latLngBox) const
{
    OsmIdSet bboxNodes;
    OsmIdSet noHupNodes;
    MultAttrMap emptyF;

    RelLst rels;
    OsmIdList ways;
    RelMap nodeRels, wayRels;

    // TODO(patrick): not needed here!
    Restrictions rests;

    NIdMap nodes;

    // always empty
    NIdMultMap multNodes;
    util::xml::XmlWriter wr(out, true, 4);

    *out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    wr.openComment();
    wr.writeText(" - written by pfaedle -");
    wr.closeTag();
    wr.openTag("osm-script");

    OsmFilter filter;

    for (const OsmReadOpts& o : opts)
    {
        filter = filter.merge(OsmFilter(o.keepFilter, o.dropFilter));
    }

    wr.openTag("union");
    size_t c = 0;
    for (auto box : latLngBox.getLeafs())
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
                    if (t == "way" && (val.second & OsmFilter::WAY)) continue;
                    if (t == "relation" && (val.second & OsmFilter::REL)) continue;
                    if (t == "node" && (val.second & OsmFilter::NODE)) continue;

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


void OsmBuilder::filterWrite(const std::string& in,
                             const std::string& out,
                             const std::vector<OsmReadOpts>& opts,
                             const BBoxIdx& latLngBox)
{
    OsmIdSet bboxNodes, noHupNodes;
    MultAttrMap emptyF;

    RelLst rels;
    OsmIdList ways;
    RelMap nodeRels, wayRels;

    // TODO(patrick): not needed here!
    Restrictions rests;

    NIdMap nodes;

    // always empty
    NIdMultMap multNodes;

    pugi::xml_document input_doc;
    input_doc.load_file(in.c_str());

    pugi::xml_document outout_doc;
    auto osm_child = outout_doc.append_child("osm");
    auto bounds_child = osm_child.append_child("bounds");
    bounds_child.append_attribute("minlat").set_value(latLngBox.getFullBox().getLowerLeft().getY());
    bounds_child.append_attribute("minlon").set_value(latLngBox.getFullBox().getLowerLeft().getX());
    bounds_child.append_attribute("maxlat").set_value(latLngBox.getFullBox().getUpperRight().getY());
    bounds_child.append_attribute("maxlon").set_value(latLngBox.getFullBox().getUpperRight().getX());


    OsmFilter filter;
    AttrKeySet attr_keys[3] = {};

    for (const OsmReadOpts& o : opts)
    {
        getKeptAttrKeys(o, attr_keys);
        filter = filter.merge(OsmFilter(o.keepFilter, o.dropFilter));
    }

    filter_nodes(input_doc, &bboxNodes, &noHupNodes, filter, latLngBox);

    readRels(input_doc, rels, nodeRels, wayRels, filter, attr_keys[2], rests);
    readEdges(input_doc, wayRels, filter, bboxNodes, attr_keys[1], ways, nodes, rels.flat);

    readWriteNds(input_doc, osm_child, nodeRels, filter, bboxNodes, nodes, attr_keys[0], rels.flat);
    readWriteWays(input_doc, osm_child, ways, attr_keys[1]);

    std::sort(ways.begin(), ways.end());
    readWriteRels(input_doc, osm_child, ways, nodes, filter, attr_keys[2]);

    std::ofstream outstr;
    outstr.open(out);
    outout_doc.save(outstr);
}


void OsmBuilder::readWriteRels(pugi::xml_document& i,
                               pugi::xml_node& o,
                               OsmIdList& ways,
                               NIdMap& nodes,
                               const OsmFilter& filter,
                               const AttrKeySet& keepAttrs)
{
    for(const auto& xmlrel : i.child("osm").children("relation"))
    {
        OsmRel rel;
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
                (keep_flags = filter.keep(rel.attrs, OsmFilter::REL)) &&
                !(drop_flags = filter.drop(rel.attrs, OsmFilter::REL)))
            {
                rel.keepFlags = keep_flags;
                rel.dropFlags = drop_flags;
            }
        }

        // processing members
        for(const auto& member: xmlrel.children("member"))
        {
            const auto& type = member.attribute("type").as_string();
            if(strcmp(type, "node") == 0)
            {
                osmid id = member.attribute("ref").as_ullong();
                rel.nodes.emplace_back(id);
                rel.nodeRoles.emplace_back(member.attribute("role").as_string());
            }
            if(strcmp(type, "way") == 0)
            {
                osmid id = member.attribute("ref").as_ullong();
                rel.ways.emplace_back(id);
                rel.wayRoles.emplace_back(member.attribute("role").as_string());
            }
        }


        OsmIdList realNodes;
        OsmIdList realWays;
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


void OsmBuilder::readWriteWays(pugi::xml_document& i,
                               pugi::xml_node& o,
                               OsmIdList& ways,
                               const AttrKeySet& keepAttrs) const
{
    NIdMultMap empty;
    for(const auto & wayxml: i.child("osm").children())
    {
        bool usable = false;
        OsmWay w;
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


NodePayload OsmBuilder::plFromGtfs(const Stop* s, const OsmReadOpts& ops)
{
    NodePayload ret(
            util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(s->getLat(), s->getLng()),
            StatInfo(ops.statNormzer.norm(s->getName()),
                     ops.trackNormzer.norm(s->getPlatformCode()), false));

#ifdef PFAEDLE_STATION_IDS
    // debug feature, store station id from GTFS
    ret.getSI()->setId(s->getId());
#endif

    if (s->getParentStation())
    {
        ret.getSI()->addAltName(
                ops.statNormzer.norm(s->getParentStation()->getName()));
    }

    return ret;
}


int OsmBuilder::filter_nodes(pugi::xml_document& xml,
                             OsmIdSet* nodes,
                             OsmIdSet* nohupNodes,
                             const OsmFilter& filter,
                             const BBoxIdx& bbox) const
{

    for(const auto& node : xml.child("osm").children("node"))
    {
        osmid cur_id = 0;

        double y = node.attribute("lat").as_double() ;
        double x = node.attribute("lon").as_double();

        if (bbox.contains(Point(x, y)))
        {
            cur_id = util::atoul(node.attribute("id").value());
            nodes->add(cur_id);
        }

        if(cur_id == 0)
            continue;

        for(const auto& tag: node.children("tag"))
        {
            if (filter.nohup(tag.attribute("k").as_string(),
                             tag.attribute("v").as_string()))
            {
                nohupNodes->add(cur_id);
            }
        }
    }

    return 0;
}


bool OsmBuilder::relKeep(osmid id, const RelMap& rels,
                         const FlatRels& fl) const
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


bool OsmBuilder::keepWay(const OsmWay& w, const RelMap& wayRels,
                         const OsmFilter& filter, const OsmIdSet& bBoxNodes,
                         const FlatRels& fl) const
{
    if (w.id && w.nodes.size() > 1 &&
        (relKeep(w.id, wayRels, fl) || filter.keep(w.attrs, OsmFilter::WAY)) &&
        !filter.drop(w.attrs, OsmFilter::WAY))
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


void OsmBuilder::readEdges(pugi::xml_document& xml,
                           const RelMap& wayRels,
                           const OsmFilter& filter,
                           const OsmIdSet& bBoxNodes,
                           const AttrKeySet& keepAttrs,
                           OsmIdList& ret,
                           NIdMap& nodes,
                           const FlatRels& flat)
{
    for(const auto& wayxml: xml.child("osm").children("way"))
    {
        OsmWay w;
        w.id = wayxml.attribute("id").as_ullong();
        for(const auto& node: wayxml.children("nd"))
        {
            osmid nid = node.attribute("ref").as_ullong();
            w.nodes.push_back(nid);

        }
        for(const auto& tag: wayxml.children("tag"))
        {
            const auto& key = tag.attribute("k").as_string();
            const auto& val = tag.attribute("v").as_string();
            if (keepAttrs.count(key))
                w.attrs[key] = val;

        }

        if(keepWay(w, wayRels, filter, bBoxNodes, flat))
        {
            ret.push_back(w.id);
            for (auto n : w.nodes)
            {
                nodes[n] = nullptr;
            }
        }
    }
}


void OsmBuilder::readEdges(pugi::xml_document& xml,
                           Graph& g,
                           const RelLst& rels,
                           const RelMap& wayRels,
                           const OsmFilter& filter,
                           const OsmIdSet& bBoxNodes,
                           NIdMap& nodes,
                           NIdMultMap& multiNodes,
                           const OsmIdSet& noHupNodes,
                           const AttrKeySet& keepAttrs,
                           const Restrictions& rawRests,
                           Restrictor& restor,
                           const FlatRels& fl,
                           EdgTracks& eTracks,
                           const OsmReadOpts& opts)
{
    for(const auto& xmlway : xml.child("osm").children("way"))
    {
        OsmWay w;
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
        if (keepWay(w, wayRels, filter, bBoxNodes, fl))
        {
            Node* last = nullptr;
            std::vector<TransitEdgeLine*> lines;
            if (wayRels.count(w.id))
            {
                lines = getLines(wayRels.find(w.id)->second, rels, opts);
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
                Node* n = nullptr;
                if (noHupNodes.has(nid))
                {
                    n = g.addNd();
                    multiNodes[nid].insert(n);
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
                    auto e = g.addEdg(last, n, EdgePayload());
                    if (!e) continue;

                    processRestr(nid, w.id, rawRests, e, n, restor);
                    processRestr(lastnid, w.id, rawRests, e, last, restor);

                    e->pl().addLines(lines);
                    e->pl().setLvl(filter.level(w.attrs));
                    if (!track.empty()) eTracks[e] = track;

                    if (filter.oneway(w.attrs)) e->pl().setOneWay(1);
                    if (filter.onewayrev(w.attrs)) e->pl().setOneWay(2);
                }
                lastnid = nid;
                last = n;
            }
        }
    }
}


void OsmBuilder::processRestr(osmid nid,
                              osmid wid,
                              const Restrictions& rawRests,
                              Edge* e,
                              Node* n,
                              Restrictor& restor) const
{
    if (rawRests.pos.count(nid))
    {
        for (const auto& kv : rawRests.pos.find(nid)->second)
        {
            if (kv.eFrom == wid)
            {
                e->pl().setRestricted();
                restor.add(e, kv.eTo, n, true);
            }
            else if (kv.eTo == wid)
            {
                e->pl().setRestricted();
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
                e->pl().setRestricted();
                restor.add(e, kv.eTo, n, false);
            }
            else if (kv.eTo == wid)
            {
                e->pl().setRestricted();
                restor.relax(wid, n, e);
            }
        }
    }
}


bool OsmBuilder::keepNode(const OsmNode& n,
                          const NIdMap& nodes,
                          const NIdMultMap& multNodes,
                          const RelMap& nodeRels,
                          const OsmIdSet& bBoxNodes,
                          const OsmFilter& filter,
                          const FlatRels& fl) const
{
    if (n.id &&
        (nodes.count(n.id) || multNodes.count(n.id) || relKeep(n.id, nodeRels, fl) || filter.keep(n.attrs, OsmFilter::NODE)) &&
        (nodes.count(n.id) || bBoxNodes.has(n.id)) &&
        (nodes.count(n.id) || multNodes.count(n.id) ||
         !filter.drop(n.attrs, OsmFilter::NODE)))
    {
        return true;
    }

    return false;
}


void OsmBuilder::readWriteNds(pugi::xml_document& i,
                              pugi::xml_node& o,
                              const RelMap& nRels,
                              const OsmFilter& filter,
                              const OsmIdSet& bBoxNds,
                              NIdMap& nds,
                              const AttrKeySet& keepAttrs,
                              const FlatRels& f) const
{
    NIdMultMap empt;
    for (const auto & xmlnode: i.child("osm").children("node"))
    {
        OsmNode nd;
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

        if (keepNode(nd, nds, empt, nRels, bBoxNds, filter, f))
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


void OsmBuilder::readNodes(pugi::xml_document& xml,
                           Graph& g,
                           const RelLst& rels,
                           const RelMap& nodeRels,
                           const OsmFilter& filter,
                           const OsmIdSet& bBoxNodes,
                           NIdMap& nodes,
                           NIdMultMap& multNodes,
                           NodeSet& orphanStations,
                           const AttrKeySet& keepAttrs,
                           const FlatRels& fl,
                           const OsmReadOpts& opts) const
{
    StAttrGroups attr_groups;

    for(const auto& xmlnode: xml.child("osm").children("node"))
    {
        OsmNode nd;
        nd.lat = xmlnode.attribute("lat").as_double();
        nd.lng = xmlnode.attribute("lon").as_double();
        nd.id = xmlnode.attribute("id").as_ullong();

        for(const auto& tag: xmlnode.children("tag"))
        {
            const auto& key = tag.attribute("k").as_string();
            const auto& val = tag.attribute("v").as_string();

            if(keepAttrs.count(key))
                nd.attrs[key] = val;
        }

        if(keepNode(nd, nodes, multNodes, nodeRels, bBoxNodes, filter, fl))
        {
            Node* n = nullptr;
            auto pos = util::geo::latLngToWebMerc<double>(nd.lat, nd.lng);
            if (nodes.count(nd.id))
            {
                n = nodes[nd.id];
                n->pl().setGeom(pos);
                if (filter.station(nd.attrs))
                {
                    auto si = getStatInfo(n, nd.id, pos, nd.attrs, &attr_groups, nodeRels,
                                          rels, opts);
                    if (!si.isNull())
                    {
                        n->pl().setSI(si);
                    }
                }
                else if (filter.blocker(nd.attrs))
                {
                    n->pl().setBlocker();
                }
            }
            else if (multNodes.count(nd.id))
            {
                for (auto* node : multNodes[nd.id])
                {
                    node->pl().setGeom(pos);
                    if (filter.station(nd.attrs))
                    {
                        auto si = getStatInfo(node, nd.id, pos, nd.attrs, &attr_groups, nodeRels,
                                              rels, opts);
                        if (!si.isNull())
                        {
                            node->pl().setSI(si);
                        }
                    }
                    else if (filter.blocker(nd.attrs))
                    {
                        node->pl().setBlocker();
                    }
                }
            }
            else
            {
                // these are nodes without any connected edges
                if (filter.station(nd.attrs))
                {
                    auto tmp = g.addNd(NodePayload(pos));
                    auto si = getStatInfo(tmp, nd.id, pos, nd.attrs, &attr_groups, nodeRels, rels, opts);

                    if (!si.isNull())
                    {
                        tmp->pl().setSI(si);
                    }

                    if (tmp->pl().getSI())
                    {
                        tmp->pl().getSI()->setIsFromOsm(false);
                        orphanStations.insert(tmp);
                    }
                }
            }
        }
    }
}


void OsmBuilder::readRels(pugi::xml_document& xml,
                          RelLst& rels,
                          RelMap& nodeRels,
                          RelMap& wayRels,
                          const OsmFilter& filter,
                          const AttrKeySet& keepAttrs,
                          Restrictions& rests) const
{
    for(const auto& xmlrel: xml.child("osm").children("relation"))
    {
        OsmRel rel;
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
                (keep_flags = filter.keep(rel.attrs, OsmFilter::REL)) &&
                !(drop_flags = filter.drop(rel.attrs, OsmFilter::REL)))
            {
                rel.keepFlags = keep_flags;
                rel.dropFlags = drop_flags;
            }

            rels.rels.emplace_back(rel.attrs);
        }

        // processing members
        for(const auto& member: xmlrel.children("member"))
        {
            const auto& type = member.attribute("type").as_string();
            if(strcmp(type, "node") == 0)
            {
                osmid id = member.attribute("ref").as_ullong();
                rel.nodes.emplace_back(id);
                rel.nodeRoles.emplace_back(member.attribute("role").as_string());
            }
            if(strcmp(type, "way") == 0)
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
        readRestr(rel, rests, filter);
    }
}


void OsmBuilder::readRestr(const OsmRel& rel,
                           Restrictions& rests,
                           const OsmFilter& filter) const
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
            if (from) return; // only one from member supported
            from = rel.ways[i];
        }
        if (rel.wayRoles[i] == "to")
        {
            if (to) return; // only one to member supported
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


std::string OsmBuilder::getAttrByFirstMatch(const DeepAttrLst& rule, osmid id,
                                            const AttrMap& attrs,
                                            const RelMap& entRels,
                                            const RelLst& rels,
                                            const Normalizer& normzer) const
{
    std::string ret;
    for (const auto& s : rule)
    {
        ret = normzer.norm(getAttr(s, id, attrs, entRels, rels));
        if (!ret.empty()) return ret;
    }
    return ret;
}


std::vector<std::string> OsmBuilder::getAttrMatchRanked(
        const DeepAttrLst& rule, osmid id, const AttrMap& attrs,
        const RelMap& entRels, const RelLst& rels,
        const Normalizer& normzer) const
{
    std::vector<std::string> ret;
    for (const auto& s : rule)
    {
        std::string tmp = normzer.norm(getAttr(s, id, attrs, entRels, rels));
        if (!tmp.empty())
        {
            ret.push_back(tmp);
        }
    }
    return ret;
}


std::string OsmBuilder::getAttr(const DeepAttrRule& s, osmid id,
                                const AttrMap& attrs, const RelMap& entRels,
                                const RelLst& rels) const
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
                if (OsmFilter::contained(rels.rels[rel_id], s.relRule.kv))
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


Nullable<StatInfo> OsmBuilder::getStatInfo(Node* node, osmid nid,
                                           const POINT& pos, const AttrMap& m,
                                           StAttrGroups* groups,
                                           const RelMap& nodeRels,
                                           const RelLst& rels,
                                           const OsmReadOpts& ops) const
{
    std::string platform;
    std::vector<std::string> names;

    names = getAttrMatchRanked(ops.statAttrRules.nameRule, nid, m, nodeRels, rels,
                               ops.statNormzer);
    platform = getAttrByFirstMatch(ops.statAttrRules.platformRule, nid, m,
                                   nodeRels, rels, ops.trackNormzer);

    if (names.empty())
        return Nullable<StatInfo>();

    auto ret = StatInfo(names[0], platform, true);

#ifdef PFAEDLE_STATION_IDS
    ret.setId(getAttrByFirstMatch(ops.statAttrRules.idRule, nid, m, nodeRels,
                                  rels, ops.idNormzer));
#endif

    for (size_t i = 1; i < names.size(); i++) ret.addAltName(names[i]);

    bool group_found = false;

    for (const auto& rule : ops.statGroupNAttrRules)
    {
        if (group_found) break;
        std::string rule_val = getAttr(rule.attr, nid, m, nodeRels, rels);
        if (!rule_val.empty())
        {
            // check if a matching group exists
            for (auto* group : (*groups)[rule.attr.attr][rule_val])
            {
                if (group_found) break;
                for (const auto* member : group->getNodes())
                {
                    if (webMercMeterDist(*member->pl().getGeom(), pos) <= rule.maxDist)
                    {
                        // ok, group is matching
                        group_found = true;
                        if (node) group->addNode(node);
                        ret.setGroup(group);
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
            std::string rule_val = getAttr(rule.attr, nid, m, nodeRels, rels);
            if (!rule_val.empty())
            {
                // add new group
                auto* g = new StatGroup();
                if (node)
                    g->addNode(node);
                ret.setGroup(g);
                (*groups)[rule.attr.attr][rule_val].push_back(g);
                break;
            }
        }
    }

    return ret;
}


double OsmBuilder::dist(const Node* a, const Node* b)
{
    return webMercMeterDist(*a->pl().getGeom(), *b->pl().getGeom());
}


double OsmBuilder::webMercDist(const Node* a, const Node* b)
{
    return webMercMeterDist(*a->pl().getGeom(), *b->pl().getGeom());
}


void OsmBuilder::writeGeoms(Graph& g)
{
    for (auto* n : *g.getNds())
    {
        for (auto* e : n->getAdjListOut())
        {
            e->pl().addPoint(*e->getFrom()->pl().getGeom());
            e->pl().setLength(dist(e->getFrom(), e->getTo()));
            e->pl().addPoint(*e->getTo()->pl().getGeom());
        }
    }
}


void OsmBuilder::fixGaps(Graph& g, NodeGrid* ng)
{
    double meter = 1;
    for (auto* n : *g.getNds())
    {
        if (n->getInDeg() + n->getOutDeg() == 1)
        {
            // get all nodes in distance
            std::set<Node*> ret;
            double distor = util::geo::webMercDistFactor(*n->pl().getGeom());
            ng->get(util::geo::pad(util::geo::getBoundingBox(*n->pl().getGeom()),
                                   meter / distor),
                    &ret);
            for (auto* nb : ret)
            {
                if (nb != n && (nb->getInDeg() + nb->getOutDeg()) == 1 &&
                    webMercDist(nb, n) <= meter / distor)
                {
                    // special case: both node are non-stations, move
                    // the end point nb to n and delete nb
                    if (!nb->pl().getSI() && !n->pl().getSI())
                    {
                        Node* otherN;
                        if (nb->getOutDeg())
                            otherN = (*nb->getAdjListOut().begin())->getOtherNd(nb);
                        else
                            otherN = (*nb->getAdjListIn().begin())->getOtherNd(nb);
                        LINE l;
                        l.push_back(*otherN->pl().getGeom());
                        l.push_back(*n->pl().getGeom());

                        Edge* e;
                        if (nb->getOutDeg())
                            e = g.addEdg(otherN, n, (*nb->getAdjListOut().begin())->pl());
                        else
                            e = g.addEdg(otherN, n, (*nb->getAdjListIn().begin())->pl());
                        if (e)
                        {
                            *e->pl().getGeom() = l;
                            g.delNd(nb);
                            ng->remove(nb);
                        }
                    }
                    else
                    {
                        // if one of the nodes is a station, just add an edge between them
                        if (nb->getOutDeg())
                            g.addEdg(n, nb, (*nb->getAdjListOut().begin())->pl());
                        else
                            g.addEdg(n, nb, (*nb->getAdjListIn().begin())->pl());
                    }
                }
            }
        }
    }
}


EdgeGrid OsmBuilder::buildEdgeIdx(Graph& g, size_t size,
                                  const BOX& webMercBox)
{
    EdgeGrid ret(size, size, webMercBox, false);
    for (auto* n : *g.getNds())
    {
        for (auto* e : n->getAdjListOut())
        {
            assert(e->pl().getGeom());
            ret.add(*e->pl().getGeom(), e);
        }
    }
    return ret;
}


NodeGrid OsmBuilder::buildNodeIdx(Graph& g, size_t size, const BOX& webMercBox,
                                  bool which)
{
    NodeGrid ret(size, size, webMercBox, false);
    for (auto* n : *g.getNds())
    {
        if (!which && n->getInDeg() + n->getOutDeg() == 1)
            ret.add(*n->pl().getGeom(), n);
        else if (which && n->pl().getSI())
            ret.add(*n->pl().getGeom(), n);
    }
    return ret;
}


Node* OsmBuilder::depthSearch(const Edge* e, const StatInfo* si, const POINT& p,
                              double maxD, int maxFullTurns, double minAngle,
                              const SearchFunc& sfunc)
{
    // shortcuts
    double dFrom = webMercMeterDist(*e->getFrom()->pl().getGeom(), p);
    double dTo = webMercMeterDist(*e->getTo()->pl().getGeom(), p);
    if (dFrom > maxD && dTo > maxD)
        return nullptr;

    if (dFrom <= maxD && sfunc(e->getFrom(), si)) return e->getFrom();
    if (dTo <= maxD && sfunc(e->getTo(), si)) return e->getTo();

    NodeCandPQ pq;
    NodeSet closed;
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
            trgraph::Node* cand;
            trgraph::Edge* edg;

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
                const POINT& toP = *cand->pl().getGeom();
                const POINT& fromP =
                        *cur.fromEdge->getOtherNd(cur.node)->pl().getGeom();
                const POINT& nodeP = *cur.node->pl().getGeom();

                if (util::geo::innerProd(nodeP, fromP, toP) < minAngle) fullTurn = 1;
            }

            if ((maxFullTurns < 0 || cur.fullTurns + fullTurn <= maxFullTurns) &&
                cur.dist + edg->pl().getLength() < maxD && !closed.count(cand))
            {
                if (sfunc(cand, si))
                {
                    return cand;
                }
                else
                {
                    pq.push(NodeCand{cur.dist + edg->pl().getLength(), cand, edg,
                                     cur.fullTurns + fullTurn});
                }
            }
        }
    }

    return nullptr;
}


bool OsmBuilder::isBlocked(const Edge* e,
                           const StatInfo* si,
                           const POINT& p,
                           double maxD,
                           int maxFullTurns,
                           double minAngle)
{
    return depthSearch(e, si, p, maxD, maxFullTurns, minAngle, BlockSearch());
}


Node* OsmBuilder::eqStatReach(const Edge* e, const StatInfo* si, const POINT& p,
                              double maxD, int maxFullTurns, double minAngle,
                              bool orphanSnap)
{
    return depthSearch(e, si, p, maxD, maxFullTurns, minAngle,
                       EqSearch(orphanSnap));
}


void OsmBuilder::getEdgCands(const POINT& geom, EdgeCandPQ& ret, EdgeGrid& eg, double d)
{
    double distor = util::geo::webMercDistFactor(geom);
    std::set<Edge*> neighs;
    BOX box = util::geo::pad(util::geo::getBoundingBox(geom), d / distor);
    eg.get(box, &neighs);

    for (auto* e : neighs)
    {
        double dist = util::geo::distToSegment(*e->getFrom()->pl().getGeom(),
                                               *e->getTo()->pl().getGeom(), geom);

        if (dist * distor <= d)
        {
            ret.push(EdgeCand(-dist, e));
        }
    }
}


std::set<Node*> OsmBuilder::getMatchingNds(const NodePayload& s, NodeGrid* ng,
                                           double d)
{
    std::set<Node*> ret;
    double distor = util::geo::webMercDistFactor(*s.getGeom());
    std::set<Node*> neighs;
    BOX box = util::geo::pad(util::geo::getBoundingBox(*s.getGeom()), d / distor);
    ng->get(box, &neighs);

    for (auto* n : neighs)
    {
        if (n->pl().getSI() && n->pl().getSI()->simi(s.getSI()) > 0.5)
        {
            double dist = webMercMeterDist(*n->pl().getGeom(), *s.getGeom());
            if (dist < d) ret.insert(n);
        }
    }

    return ret;
}


Node* OsmBuilder::getMatchingNd(const NodePayload& s, NodeGrid& ng, double d)
{
    double distor = util::geo::webMercDistFactor(*s.getGeom());
    std::set<Node*> neighs;
    BOX box = util::geo::pad(util::geo::getBoundingBox(*s.getGeom()), d / distor);
    ng.get(box, &neighs);

    Node* ret = nullptr;
    double best_d = std::numeric_limits<double>::max();

    for (auto* n : neighs)
    {
        if (n->pl().getSI() && n->pl().getSI()->simi(s.getSI()) > 0.5)
        {
            double dist = webMercMeterDist(*n->pl().getGeom(), *s.getGeom());
            if (dist < d && dist < best_d)
            {
                best_d = dist;
                ret = n;
            }
        }
    }

    return ret;
}


std::set<Node*> OsmBuilder::snapStation(Graph& g,
                                        NodePayload& s,
                                        EdgeGrid& eg,
                                        NodeGrid& sng,
                                        const OsmReadOpts& opts,
                                        Restrictor& restor,
                                        bool surrHeur,
                                        bool orphSnap,
                                        double d)
{
    assert(s.getSI());
    std::set<Node*> ret;

    EdgeCandPQ pq;

    getEdgCands(*s.getGeom(), pq, eg, d);

    if (pq.empty() && surrHeur)
    {
        // no station found in the first round, try again with the nearest
        // surrounding station with matching name
        const Node* best = getMatchingNd(s, sng, opts.maxSnapFallbackHeurDistance);
        if (best)
        {
            getEdgCands(*best->pl().getGeom(), pq, eg, d);
        }
        else
        {
            // if still no luck, get edge cands in fallback snap distance
            getEdgCands(*s.getGeom(), pq, eg, opts.maxSnapFallbackHeurDistance);
        }
    }

    while (!pq.empty())
    {
        auto* e = pq.top().second;
        pq.pop();
        auto geom = util::geo::projectOn(*e->getFrom()->pl().getGeom(),
                                         *s.getGeom(),
                                         *e->getTo()->pl().getGeom());

        Node* eq = nullptr;
        if (!(eq = eqStatReach(e, s.getSI(), geom, 2 * d, 0,
                               opts.maxAngleSnapReach, orphSnap)))
        {
            if (e->pl().lvl() > opts.maxSnapLevel) continue;
            if (isBlocked(e,
                          s.getSI(),
                          geom,
                          opts.maxBlockDistance,
                          0,
                          opts.maxAngleSnapReach))
            {
                continue;
            }

            // if the projected position is near (< 2 meters) the end point of this
            // way and the endpoint is not already a station, place the station there.
            if (!e->getFrom()->pl().getSI() && webMercMeterDist(geom, *e->getFrom()->pl().getGeom()) < 2)
            {
                e->getFrom()->pl().setSI(*s.getSI());

                if (s.getSI()->getGroup())
                    s.getSI()->getGroup()->addNode(e->getFrom());

                ret.insert(e->getFrom());
            }
            else if (!e->getTo()->pl().getSI() && webMercMeterDist(geom, *e->getTo()->pl().getGeom()) < 2)
            {
                e->getTo()->pl().setSI(*s.getSI());

                if (s.getSI()->getGroup())
                    s.getSI()->getGroup()->addNode(e->getTo());

                ret.insert(e->getTo());
            }
            else
            {
                s.setGeom(geom);
                Node* n = g.addNd(s);

                if (n->pl().getSI()->getGroup())
                    n->pl().getSI()->getGroup()->addNode(n);

                sng.add(geom, n);

                auto ne = g.addEdg(e->getFrom(), n, e->pl());
                ne->pl().setLength(webMercDist(n, e->getFrom()));
                LINE l;
                l.push_back(*e->getFrom()->pl().getGeom());
                l.push_back(*n->pl().getGeom());
                *ne->pl().getGeom() = l;
                eg.add(l, ne);

                auto nf = g.addEdg(n, e->getTo(), e->pl());
                nf->pl().setLength(webMercDist(n, e->getTo()));
                LINE ll;
                ll.push_back(*n->pl().getGeom());
                ll.push_back(*e->getTo()->pl().getGeom());
                *nf->pl().getGeom() = ll;
                eg.add(ll, nf);

                // replace edge in restrictor
                restor.replaceEdge(e, ne, nf);

                g.delEdg(e->getFrom(), e->getTo());
                eg.remove(e);
                ret.insert(n);
            }
        }
        else
        {
            // if the snapped station is very near to the original OSM station
            // write additional info from this snap station to the equivalent stat
            if (webMercMeterDist(*s.getGeom(), *eq->pl().getGeom()) < opts.maxOsmStationDistance)
            {
                if (eq->pl().getSI()->getTrack().empty())
                    eq->pl().getSI()->setTrack(s.getSI()->getTrack());
            }
            ret.insert(eq);
        }
    }

    return ret;
}


StatGroup* OsmBuilder::groupStats(const NodeSet& s)
{
    if (s.empty())
        return nullptr;
    // reference group
    auto* ret = new StatGroup();
    bool used = false;

    for (auto* n : s)
    {
        if (!n->pl().getSI())
            continue;
        used = true;
        if (n->pl().getSI()->getGroup())
        {
            // this node is already in a group - merge this group with this one
            ret->merge(n->pl().getSI()->getGroup());
        }
        else
        {
            ret->addNode(n);
            n->pl().getSI()->setGroup(ret);
        }
    }

    if (!used)
    {
        delete ret;
        return nullptr;
    }

    return ret;
}


std::vector<TransitEdgeLine*> OsmBuilder::getLines(
        const std::vector<size_t>& edgeRels,
        const RelLst& rels,
        const OsmReadOpts& ops)
{
    std::vector<TransitEdgeLine*> ret;
    for (size_t rel_id : edgeRels)
    {
        TransitEdgeLine* elp = nullptr;

        if (_relLines.count(rel_id))
        {
            elp = _relLines[rel_id];
        }
        else
        {
            TransitEdgeLine el;

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
                elp = new TransitEdgeLine(el);
                _lines[el] = elp;
                _relLines[rel_id] = elp;
            }
        }
        ret.push_back(elp);
    }
    return ret;
}


void OsmBuilder::getKeptAttrKeys(const OsmReadOpts& opts,
                                 AttrKeySet sets[3]) const
{
    for (const auto& i : opts.statGroupNAttrRules)
    {
        if (i.attr.relRule.kv.first.empty())
        {
            sets[0].insert(i.attr.attr);
        }
        else
        {
            sets[2].insert(i.attr.relRule.kv.first);
            sets[2].insert(i.attr.attr);
        }
    }

    for (const auto& i : opts.keepFilter)
    {
        for (size_t j = 0; j < 3; j++) sets[j].insert(i.first);
    }

    for (const auto& i : opts.dropFilter)
    {
        for (size_t j = 0; j < 3; j++) sets[j].insert(i.first);
    }

    for (const auto& i : opts.noHupFilter)
    {
        sets[0].insert(i.first);
    }

    for (const auto& i : opts.oneWayFilter)
    {
        sets[1].insert(i.first);
    }

    for (const auto& i : opts.oneWayFilterRev)
    {
        sets[1].insert(i.first);
    }

    for (const auto& i : opts.twoWayFilter)
    {
        sets[1].insert(i.first);
    }

    for (const auto& i : opts.stationFilter)
    {
        sets[0].insert(i.first);
        sets[2].insert(i.first);
    }

    for (const auto& i : opts.stationBlockerFilter)
    {
        sets[0].insert(i.first);
    }

    for (uint8_t j = 0; j < 7; j++)
    {
        for (const auto& kv : *(opts.levelFilters + j))
        {
            sets[1].insert(kv.first);
        }
    }

    // restriction system
    for (const auto& i : opts.restrPosRestr)
    {
        sets[2].insert(i.first);
    }
    for (const auto& i : opts.restrNegRestr)
    {
        sets[2].insert(i.first);
    }
    for (const auto& i : opts.noRestrFilter)
    {
        sets[2].insert(i.first);
    }

    sets[2].insert("from");
    sets[2].insert("via");
    sets[2].insert("to");

    sets[2].insert(opts.relLinerules.toNameRule.begin(),
                   opts.relLinerules.toNameRule.end());
    sets[2].insert(opts.relLinerules.fromNameRule.begin(),
                   opts.relLinerules.fromNameRule.end());
    sets[2].insert(opts.relLinerules.sNameRule.begin(),
                   opts.relLinerules.sNameRule.end());

    for (const auto& i : opts.statAttrRules.nameRule)
    {
        if (i.relRule.kv.first.empty())
        {
            sets[0].insert(i.attr);
        }
        else
        {
            sets[2].insert(i.relRule.kv.first);
            sets[2].insert(i.attr);
        }
    }

    for (const auto& i : opts.edgePlatformRules)
    {
        if (i.relRule.kv.first.empty())
        {
            sets[1].insert(i.attr);
        }
        else
        {
            sets[2].insert(i.relRule.kv.first);
            sets[2].insert(i.attr);
        }
    }

    for (const auto& i : opts.statAttrRules.platformRule)
    {
        if (i.relRule.kv.first.empty())
        {
            sets[0].insert(i.attr);
        }
        else
        {
            sets[2].insert(i.relRule.kv.first);
            sets[2].insert(i.attr);
        }
    }

    for (const auto& i : opts.statAttrRules.idRule)
    {
        if (i.relRule.kv.first.empty())
        {
            sets[0].insert(i.attr);
        }
        else
        {
            sets[2].insert(i.relRule.kv.first);
            sets[2].insert(i.attr);
        }
    }
}


void OsmBuilder::deleteOrphEdgs(Graph& g, const OsmReadOpts& opts)
{
    const size_t rounds = 3;
    for (size_t c = 0; c < rounds; c++)
    {
        for (auto i = g.getNds()->begin(); i != g.getNds()->end();)
        {
            if ((*i)->getInDeg() + (*i)->getOutDeg() != 1 || (*i)->pl().getSI())
            {
                ++i;
                continue;
            }

            // check if the removal of this edge would transform a steep angle
            // full turn at an intersection into a node 2 eligible for contraction
            // if so, dont delete
            if (keepFullTurn(*i, opts.fullTurnAngle))
            {
                ++i;
                continue;
            }

            i = g.delNd(*i);
            continue;
            i++;
        }
    }
}


void OsmBuilder::deleteOrphNds(Graph& g)
{
    for (auto i = g.getNds()->begin(); i != g.getNds()->end();)
    {
        if ((*i)->getInDeg() + (*i)->getOutDeg() == 0 &&
            !((*i)->pl().getSI() && (*i)->pl().getSI()->getGroup()))
        {
            i = g.delNd(i);
            // TODO(patrick): maybe delete from node grid?
        }
        else
        {
            i++;
        }
    }
}


bool OsmBuilder::edgesSim(const Edge* a, const Edge* b)
{
    if (static_cast<bool>(a->pl().oneWay()) ^ static_cast<bool>(b->pl().oneWay()))
        return false;
    if (a->pl().lvl() != b->pl().lvl())
        return false;
    if (a->pl().getLines().size() != b->pl().getLines().size())
        return false;
    if (a->pl().getLines() != b->pl().getLines())
        return false;

    if (a->pl().oneWay() && b->pl().oneWay())
    {
        if (a->getFrom() != b->getTo() && a->getTo() != b->getFrom())
            return false;
    }
    if (a->pl().isRestricted() || b->pl().isRestricted())
        return false;

    return true;
}


const EdgePayload& OsmBuilder::mergeEdgePL(Edge* a, Edge* b)
{
    const Node* n = nullptr;
    if (a->getFrom() == b->getFrom())
        n = a->getFrom();
    else if (a->getFrom() == b->getTo())
        n = a->getFrom();
    else
        n = a->getTo();

    if (a->getTo() == n && b->getTo() == n)
    {
        // --> n <--
        a->pl().getGeom()->insert(a->pl().getGeom()->end(),
                                  b->pl().getGeom()->rbegin(),
                                  b->pl().getGeom()->rend());
    }
    else if (a->getTo() == n && b->getFrom() == n)
    {
        // --> n -->
        a->pl().getGeom()->insert(a->pl().getGeom()->end(),
                                  b->pl().getGeom()->begin(),
                                  b->pl().getGeom()->end());
    }
    else if (a->getFrom() == n && b->getTo() == n)
    {
        // <-- n <--
        std::reverse(a->pl().getGeom()->begin(), a->pl().getGeom()->end());
        a->pl().getGeom()->insert(a->pl().getGeom()->end(),
                                  b->pl().getGeom()->rbegin(),
                                  b->pl().getGeom()->rend());
    }
    else
    {
        // <-- n -->
        std::reverse(a->pl().getGeom()->begin(), a->pl().getGeom()->end());
        a->pl().getGeom()->insert(a->pl().getGeom()->end(),
                                  b->pl().getGeom()->begin(),
                                  b->pl().getGeom()->end());
    }

    a->pl().setLength(a->pl().getLength() + b->pl().getLength());

    return a->pl();
}


void OsmBuilder::collapseEdges(Graph& g)
{
    for (auto* n : *g.getNds())
    {
        if (n->getOutDeg() + n->getInDeg() != 2 || n->pl().getSI()) continue;

        Edge* ea;
        Edge* eb;
        if (n->getOutDeg() == 2)
        {
            ea = *n->getAdjListOut().begin();
            eb = *n->getAdjListOut().rbegin();
        }
        else if (n->getInDeg() == 2)
        {
            ea = *n->getAdjListIn().begin();
            eb = *n->getAdjListIn().rbegin();
        }
        else
        {
            ea = *n->getAdjListOut().begin();
            eb = *n->getAdjListIn().begin();
        }

        // important, we don't have a multigraph! if the same edge
        // will already exist, leave this node
        if (g.getEdg(ea->getOtherNd(n), eb->getOtherNd(n))) continue;
        if (g.getEdg(eb->getOtherNd(n), ea->getOtherNd(n))) continue;

        if (edgesSim(ea, eb))
        {
            if (ea->pl().oneWay() && ea->getOtherNd(n) != ea->getFrom())
            {
                g.addEdg(eb->getOtherNd(n), ea->getOtherNd(n), mergeEdgePL(eb, ea));
            }
            else
            {
                g.addEdg(ea->getOtherNd(n), eb->getOtherNd(n), mergeEdgePL(ea, eb));
            }
            g.delEdg(ea->getFrom(), ea->getTo());
            g.delEdg(eb->getFrom(), eb->getTo());
        }
    }
}


void OsmBuilder::simplifyGeoms(Graph& g)
{
    for (auto* n : *g.getNds())
    {
        for (auto* e : n->getAdjListOut())
        {
            (*e->pl().getGeom()) = util::geo::simplify(*e->pl().getGeom(), 0.5);
        }
    }
}


uint32_t OsmBuilder::writeComps(Graph& g)
{
    auto* comp = new Component{7};
    uint32_t comp_counter = 0;

    for (auto* n : *g.getNds())
    {
        if (!n->pl().getComp())
        {
            using node_edge_pair = std::pair<Node*, Edge*>;
            std::stack<node_edge_pair> stack({node_edge_pair(n, 0)});
            while (!stack.empty())
            {
                auto stack_top = stack.top();
                stack.pop();

                stack_top.first->pl().setComp(comp);
                for (auto* e : stack_top.first->getAdjListOut())
                {
                    if (e->pl().lvl() < comp->minEdgeLvl)
                        comp->minEdgeLvl = e->pl().lvl();
                    if (!e->getOtherNd(stack_top.first)->pl().getComp())
                        stack.emplace(e->getOtherNd(stack_top.first), e);
                }
                for (auto* e : stack_top.first->getAdjListIn())
                {
                    if (e->pl().lvl() < comp->minEdgeLvl)
                        comp->minEdgeLvl = e->pl().lvl();
                    if (!e->getOtherNd(stack_top.first)->pl().getComp())
                        stack.emplace(e->getOtherNd(stack_top.first), e);
                }
            }

            comp_counter++;
            comp = new Component{7};
        }
    }

    // the last comp was not used
    delete comp;

    return comp_counter;
}


void OsmBuilder::writeEdgeTracks(const EdgTracks& tracks)
{
    for (const auto& tr : tracks)
    {
        if (tr.first->getTo()->pl().getSI() &&
            tr.first->getTo()->pl().getSI()->getTrack().empty())
        {
            tr.first->getTo()->pl().getSI()->setTrack(tr.second);
        }
        if (tr.first->getFrom()->pl().getSI() &&
            tr.first->getFrom()->pl().getSI()->getTrack().empty())
        {
            tr.first->getFrom()->pl().getSI()->setTrack(tr.second);
        }
    }
}


void OsmBuilder::writeODirEdgs(Graph& g, Restrictor& restor)
{
    for (auto* n : *g.getNds())
    {
        for (auto* e : n->getAdjListOut())
        {
            if (g.getEdg(e->getTo(), e->getFrom()))
                continue;
            auto newE = g.addEdg(e->getTo(), e->getFrom(), e->pl().revCopy());
            if (e->pl().isRestricted())
                restor.duplicateEdge(e, newE);
        }
    }
}


void OsmBuilder::writeSelfEdgs(Graph& g)
{
    for (auto* n : *g.getNds())
    {
        if (n->pl().getSI() && n->getAdjListOut().empty())
        {
            g.addEdg(n, n);
        }
    }
}


bool OsmBuilder::keepFullTurn(const trgraph::Node* n, double ang)
{
    if (n->getInDeg() + n->getOutDeg() != 1) return false;

    const trgraph::Edge* e = nullptr;
    if (n->getOutDeg())
        e = n->getAdjListOut().front();
    else
        e = n->getAdjListIn().front();

    auto other = e->getOtherNd(n);

    if (other->getInDeg() + other->getOutDeg() == 3)
    {
        const trgraph::Edge* a = nullptr;
        const trgraph::Edge* b = nullptr;
        for (auto f : other->getAdjListIn())
        {
            if (f != e && !a)
                a = f;
            else if (f != e && !b)
                b = f;
        }

        for (auto f : other->getAdjListOut())
        {
            if (f != e && !a)
                a = f;
            else if (f != e && !b)
                b = f;
        }

        auto ap = a->pl().backHop();
        auto bp = b->pl().backHop();
        if (a->getTo() != other) ap = a->pl().frontHop();
        if (b->getTo() != other) bp = b->pl().frontHop();

        return router::angSmaller(ap, *other->pl().getGeom(), bp, ang);
    }

    return false;
}


void OsmBuilder::snapStats(const OsmReadOpts& opts,
                           Graph& g,
                           const BBoxIdx& bbox,
                           size_t gridSize,
                           router::FeedStops& fs,
                           Restrictor& res,
                           const NodeSet& orphanStations)
{
    NodeGrid sng = buildNodeIdx(g, gridSize, bbox.getFullWebMercBox(), true);
    EdgeGrid eg = buildEdgeIdx(g, gridSize, bbox.getFullWebMercBox());

    LOG(DEBUG) << "Grid size of " << sng.getXWidth() << "x" << sng.getYHeight();

    for (double d : opts.maxSnapDistances)
    {
        for (auto s : orphanStations)
        {
            POINT geom = *s->pl().getGeom();
            NodePayload pl = s->pl();
            pl.getSI()->setIsFromOsm(false);
            const auto& r = snapStation(g, pl, eg, sng, opts, res, false, false, d);
            groupStats(r);
            for (auto n : r)
            {
                // if the snapped station is very near to the original OSM
                // station, set is-from-osm to true
                if (webMercMeterDist(geom, *n->pl().getGeom()) < opts.maxOsmStationDistance)
                {
                    if (n->pl().getSI()) n->pl().getSI()->setIsFromOsm(true);
                }
            }
        }
    }

    std::vector<const Stop*> notSnapped;

    for (auto& s : fs)
    {
        bool snapped = false;
        auto pl = plFromGtfs(s.first, opts);
        for (size_t i = 0; i < opts.maxSnapDistances.size(); i++)
        {
            double d = opts.maxSnapDistances[i];

            StatGroup* group = groupStats(
                    snapStation(g, pl, eg, sng, opts, res,
                                i == opts.maxSnapDistances.size() - 1, false, d));

            if (group)
            {
                group->addStop(s.first);
                fs[s.first] = *group->getNodes().begin();
                snapped = true;
            }
        }
        if (!snapped)
        {
            LOG(TRACE) << "Could not snap station "
                        << "(" << pl.getSI()->getName() << ")"
                        << " (" << s.first->getLat() << "," << s.first->getLng()
                        << ") in normal run, trying again later in orphan mode.";
            if (!bbox.contains(*pl.getGeom()))
            {
                LOG(TRACE) << "Note: '" << pl.getSI()->getName()
                            << "' does not lie within the bounds for this graph and "
                               "may be a stray station";
            }
            notSnapped.push_back(s.first);
        }
    }

    if (!notSnapped.empty())
        LOG(TRACE) << notSnapped.size()
                    << " stations could not be snapped in "
                       "normal run, trying again in orphan "
                       "mode.";

    // try again, but aggressively snap to orphan OSM stations which have
    // not been assigned to any GTFS stop yet
    for (auto& s : notSnapped)
    {
        bool snapped = false;
        auto pl = plFromGtfs(s, opts);
        for (size_t i = 0; i < opts.maxSnapDistances.size(); i++)
        {
            double d = opts.maxSnapDistances[i];

            StatGroup* group = groupStats(
                    snapStation(g, pl, eg, sng, opts, res,
                                i == opts.maxSnapDistances.size() - 1, true, d));

            if (group)
            {
                group->addStop(s);
                // add the added station name as an alt name to ensure future
                // similarity
                for (auto n : group->getNodes())
                {
                    if (n->pl().getSI())
                        n->pl().getSI()->addAltName(pl.getSI()->getName());
                }
                fs[s] = *group->getNodes().begin();
                snapped = true;
            }
        }
        if (!snapped)
        {
            // finally give up

            // add a group with only this stop in it
            auto* dummy_group = new StatGroup();
            auto* dummy_node = g.addNd(pl);

            dummy_node->pl().getSI()->setGroup(dummy_group);
            dummy_group->addNode(dummy_node);
            dummy_group->addStop(s);
            fs[s] = dummy_node;
            if (!bbox.contains(*pl.getGeom()))
            {
                LOG(TRACE) << "Could not snap station "
                            << "(" << pl.getSI()->getName() << ")"
                            << " (" << s->getLat() << "," << s->getLng() << ")";
                LOG(TRACE) << "Note: '" << pl.getSI()->getName()
                            << "' does not lie within the bounds for this graph and "
                               "may be a stray station";
            }
            else
            {
                // only warn if it is contained in the BBOX for this graph
                LOG(WARN) << "Could not snap station "
                          << "(" << pl.getSI()->getName() << ")"
                          << " (" << s->getLat() << "," << s->getLng() << ")";
            }
        }
    }
}
