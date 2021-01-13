// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_OSMBUILDER_H_
#define PFAEDLE_OSM_OSMBUILDER_H_

#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/definitions.h"
#include "pfaedle/osm/BBoxIdx.h"
#include "pfaedle/osm/OsmFilter.h"
#include "pfaedle/osm/OsmIdSet.h"
#include "pfaedle/osm/OsmReadOpts.h"
#include "pfaedle/osm/Restrictor.h"
#include "pfaedle/router/router.h"
#include "pfaedle/trgraph/graph.h"
#include "pfaedle/trgraph/node_payload.h"
#include "pfaedle/trgraph/normalizer.h"
#include "pfaedle/trgraph/station_info.h"
#include "util/geo/Geo.h"
#include "util/xml/XmlWriter.h"

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

namespace pfaedle::osm
{

using ad::cppgtfs::gtfs::Stop;

struct NodeCand
{
    double dist;
    trgraph::node* node;
    const trgraph::edge* fromEdge;
    int fullTurns;
};

struct SearchFunc
{
    virtual bool operator()(const trgraph::node* n, const trgraph::station_info* si) const = 0;
};

struct EqSearch : public SearchFunc
{
    explicit EqSearch(bool orphan_snap) :
        orphanSnap(orphan_snap) {}
    double minSimi = 0.9;
    bool orphanSnap;
    bool operator()(const trgraph::node* cand, const trgraph::station_info* si) const override;
};

struct BlockSearch : public SearchFunc
{
    bool operator()(const trgraph::node* n, const trgraph::station_info* si) const override
    {
        if (n->pl().getSI() && n->pl().getSI()->simi(si) < 0.5) return true;
        return n->pl().isBlocker();
    }
};

inline bool operator<(const NodeCand& a, const NodeCand& b)
{
    return a.fullTurns > b.fullTurns || a.dist > b.dist;
}

using NodeCandPQ = std::priority_queue<NodeCand>;

/*
 * Builds a physical transit network graph from OSM data
 */
class OsmBuilder
{
public:
    OsmBuilder();

    // Read the OSM file at path, and write a graph to g. Only elements
    // inside the bounding box will be read
    void read(const std::string& path,
              const OsmReadOpts& opts,
              trgraph::graph& g,
              const BBoxIdx& box,
              size_t gridSize,
              router::feed_stops& fs,
              Restrictor& res);

    // Based on the list of options, output an overpass XML query for getting
    // the data needed for routing
    void overpassQryWrite(std::ostream& out,
                          const std::vector<OsmReadOpts>& opts,
                          const BBoxIdx& latLngBox) const;

    // Based on the list of options, read an OSM file from in and output an
    // OSM file to out which contains exactly the entities that are needed
    // from the file at in
    void filterWrite(const std::string& in,
                     const std::string& out,
                     const std::vector<OsmReadOpts>& opts,
                     const BBoxIdx& box);

private:
    int filter_nodes(pugi::xml_document& xml, OsmIdSet& nodes,
                     OsmIdSet& noHupNodes, const OsmFilter& filter,
                     const BBoxIdx& bbox) const;

    void readRels(pugi::xml_document& xml,
                  RelLst& rels,
                  RelMap& nodeRels,
                  RelMap& wayRels,
                  const OsmFilter& filter,
                  const AttrKeySet& keepAttrs,
                  Restrictions& rests) const;

    void readRestr(const OsmRel& rel,
                   Restrictions& rests,
                   const OsmFilter& filter) const;

    void readNodes(pugi::xml_document& f,
                   trgraph::graph& g,
                   const RelLst& rels,
                   const RelMap& nodeRels,
                   const OsmFilter& filter,
                   const OsmIdSet& bBoxNodes,
                   NIdMap& nodes,
                   NIdMultMap& multNodes,
                   router::node_set& orphanStations,
                   const AttrKeySet& keepAttrs,
                   const FlatRels& flatRels,
                   const OsmReadOpts& opts) const;

    void readWriteNds(pugi::xml_document& i,
                      pugi::xml_node& o,
                      const RelMap& nodeRels,
                      const OsmFilter& filter,
                      const OsmIdSet& bBoxNodes,
                      NIdMap& nodes,
                      const AttrKeySet& keepAttrs,
                      const FlatRels& f) const;

    void readWriteWays(pugi::xml_document& i,
                       pugi::xml_node& o,
                       OsmIdList& ways,
                       const AttrKeySet& keepAttrs) const;

    void readWriteRels(pugi::xml_document& i,
                       pugi::xml_node& o,
                       OsmIdList& ways,
                       NIdMap& nodes,
                       const OsmFilter& filter,
                       const AttrKeySet& keepAttrs);

    void readEdges(pugi::xml_document& xml,
                   trgraph::graph& g, const RelLst& rels,
                   const RelMap& wayRels,
                   const OsmFilter& filter,
                   const OsmIdSet& bBoxNodes,
                   NIdMap& nodes,
                   NIdMultMap& multNodes,
                   const OsmIdSet& noHupNodes,
                   const AttrKeySet& keepAttrs,
                   const Restrictions& rest,
                   Restrictor& restor,
                   const FlatRels& flatRels,
                   EdgTracks& etracks,
                   const OsmReadOpts& opts);

    void readEdges(pugi::xml_document& xml,
                   const RelMap& wayRels,
                   const OsmFilter& filter,
                   const OsmIdSet& bBoxNodes,
                   const AttrKeySet& keepAttrs,
                   OsmIdList& ret,
                   NIdMap& nodes,
                   const FlatRels& flatRels);

    bool keepWay(const OsmWay& w, const RelMap& wayRels, const OsmFilter& filter,
                 const OsmIdSet& bBoxNodes, const FlatRels& fl) const;


    bool keepNode(const OsmNode& n, const NIdMap& nodes,
                  const NIdMultMap& multNodes, const RelMap& nodeRels,
                  const OsmIdSet& bBoxNodes, const OsmFilter& filter,
                  const FlatRels& fl) const;

    std::optional<trgraph::station_info> getStatInfo(trgraph::node* node, osmid nid, const POINT& pos,
                                   const AttrMap& m, StAttrGroups* groups,
                                   const RelMap& nodeRels, const RelLst& rels,
                                   const OsmReadOpts& ops) const;

    static void snapStats(const OsmReadOpts& opts,
                          trgraph::graph& g,
                          const BBoxIdx& bbox,
                          size_t gridSize,
                          router::feed_stops& fs,
                          Restrictor& res,
                          const router::node_set& orphanStations);
    static void writeGeoms(trgraph::graph& g);
    static void deleteOrphNds(trgraph::graph& g);
    static void deleteOrphEdgs(trgraph::graph& g, const OsmReadOpts& opts);
    static double dist(const trgraph::node* a, const trgraph::node* b);
    static double webMercDist(const trgraph::node* a, const trgraph::node* b);

    static trgraph::node_grid buildNodeIdx(trgraph::graph& g, size_t size, const BOX& webMercBox,
                                 bool which);

    static trgraph::edge_grid buildEdgeIdx(trgraph::graph& g, size_t size, const BOX& webMercBox);

    static void fixGaps(trgraph::graph& g, trgraph::node_grid* ng);
    static void collapseEdges(trgraph::graph& g);
    static void writeODirEdgs(trgraph::graph& g, Restrictor& restor);
    static void writeSelfEdgs(trgraph::graph& g);
    static void writeEdgeTracks(const EdgTracks& tracks);
    static void simplifyGeoms(trgraph::graph& g);
    static uint32_t writeComps(trgraph::graph& g);
    static bool edgesSim(const trgraph::edge* a, const trgraph::edge* b);
    static const trgraph::edge_payload& mergeEdgePL(trgraph::edge* a, trgraph::edge* b);
    static void getEdgCands(const POINT& s, EdgeCandPQ& ret, trgraph::edge_grid& eg, double d);

    static std::set<trgraph::node*> getMatchingNds(const trgraph::node_payload& s, trgraph::node_grid* ng,
                                          double d);

    static trgraph::node* getMatchingNd(const trgraph::node_payload& s, trgraph::node_grid& ng, double d);

    static router::node_set snapStation(trgraph::graph& g, trgraph::node_payload& s, trgraph::edge_grid& eg, trgraph::node_grid& sng,
                               const OsmReadOpts& opts, Restrictor& restor,
                               bool surHeur, bool orphSnap, double maxD);

    // Checks if from the edge e, a station similar to si can be reach with less
    // than maxD distance and less or equal to "maxFullTurns" full turns. If
    // such a station exists, it is returned. If not, 0 is returned.
    static trgraph::node* eqStatReach(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
                             double maxD, int maxFullTurns, double maxAng,
                             bool orph);

    static trgraph::node* depthSearch(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
                             double maxD, int maxFullTurns, double minAngle,
                             const SearchFunc& sfunc);

    static bool isBlocked(const trgraph::edge* e, const trgraph::station_info* si, const POINT& p,
                          double maxD, int maxFullTurns, double minAngle);
    static bool keepFullTurn(const trgraph::node* n, double ang);

    static trgraph::station_group* groupStats(const router::node_set& s);

    static trgraph::node_payload plFromGtfs(const Stop* s, const OsmReadOpts& ops);

    std::vector<trgraph::transit_edge_line*> getLines(const std::vector<size_t>& edgeRels,
                                           const RelLst& rels,
                                           const OsmReadOpts& ops);

    void getKeptAttrKeys(const OsmReadOpts& opts, AttrKeySet sets[3]) const;


    void processRestr(osmid nid, osmid wid, const Restrictions& rawRests, trgraph::edge* e,
                      trgraph::node* n, Restrictor& restor) const;

    std::string getAttrByFirstMatch(const DeepAttrLst& rule, osmid id,
                                    const AttrMap& attrs, const RelMap& entRels,
                                    const RelLst& rels,
                                    const trgraph::normalizer& norm) const;

    std::vector<std::string> getAttrMatchRanked(const DeepAttrLst& rule, osmid id,
                                                const AttrMap& attrs,
                                                const RelMap& entRels,
                                                const RelLst& rels,
                                                const trgraph::normalizer& norm) const;

    std::string getAttr(const DeepAttrRule& s, osmid id, const AttrMap& attrs,
                        const RelMap& entRels, const RelLst& rels) const;

    bool relKeep(osmid id, const RelMap& rels, const FlatRels& fl) const;

    std::map<trgraph::transit_edge_line, trgraph::transit_edge_line*> _lines;
    std::map<size_t, trgraph::transit_edge_line*> _relLines;
};
}  // namespace pfaedle::osm
#endif  // PFAEDLE_OSM_OSMBUILDER_H_
