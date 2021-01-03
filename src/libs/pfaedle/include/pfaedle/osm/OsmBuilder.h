// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_OSMBUILDER_H_
#define PFAEDLE_OSM_OSMBUILDER_H_

#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/Def.h"
#include "pfaedle/osm/BBoxIdx.h"
#include "pfaedle/osm/OsmFilter.h"
#include "pfaedle/osm/OsmIdSet.h"
#include "pfaedle/osm/OsmReadOpts.h"
#include "pfaedle/osm/Restrictor.h"
#include "pfaedle/router/Router.h"
#include "pfaedle/trgraph/Graph.h"
#include "pfaedle/trgraph/Normalizer.h"
#include "pfaedle/trgraph/StatInfo.h"
#include "util/Nullable.h"
#include "util/geo/Geo.h"
#include "util/xml/XmlWriter.h"

#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pugi{
class xml_document;
class xml_node;
}

namespace pfaedle::osm
{

using pfaedle::trgraph::EdgeGrid;
using pfaedle::trgraph::NodeGrid;
using pfaedle::trgraph::Normalizer;
using pfaedle::trgraph::Graph;
using pfaedle::trgraph::Node;
using pfaedle::trgraph::NodePL;
using pfaedle::trgraph::Edge;
using pfaedle::trgraph::EdgePL;
using pfaedle::trgraph::TransitEdgeLine;
using pfaedle::trgraph::StatInfo;
using pfaedle::trgraph::StatGroup;
using pfaedle::trgraph::Component;
using pfaedle::router::NodeSet;
using ad::cppgtfs::gtfs::Stop;
using util::Nullable;

struct NodeCand
{
    double dist;
    Node* node;
    const Edge* fromEdge;
    int fullTurns;
};

struct SearchFunc
{
    virtual bool operator()(const Node* n, const StatInfo* si) const = 0;
};

struct EqSearch : public SearchFunc
{
    explicit EqSearch(bool orphan_snap) :
        orphanSnap(orphan_snap) {}
    double minSimi = 0.9;
    bool orphanSnap;
    bool operator()(const Node* cand, const StatInfo* si) const override;
};

struct BlockSearch : public SearchFunc
{
    bool operator()(const Node* n, const StatInfo* si) const override
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
              Graph& g,
              const BBoxIdx& box,
              size_t gridSize,
              router::FeedStops& fs,
              Restrictor& res);

    // Based on the list of options, output an overpass XML query for getting
    // the data needed for routing
    void overpassQryWrite(std::ostream* out, const std::vector<OsmReadOpts>& opts,
                          const BBoxIdx& latLngBox) const;

    // Based on the list of options, read an OSM file from in and output an
    // OSM file to out which contains exactly the entities that are needed
    // from the file at in
    void filterWrite(const std::string& in, const std::string& out,
                     const std::vector<OsmReadOpts>& opts, const BBoxIdx& box);

private:
    int filter_nodes(pugi::xml_document& xml, OsmIdSet* nodes,
                     OsmIdSet* noHupNodes, const OsmFilter& filter,
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
                   Graph& g,
                   const RelLst& rels,
                   const RelMap& nodeRels,
                   const OsmFilter& filter,
                   const OsmIdSet& bBoxNodes,
                   NIdMap& nodes,
                   NIdMultMap& multNodes,
                   NodeSet& orphanStations,
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
                   Graph& g, const RelLst& rels,
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

    Nullable<StatInfo> getStatInfo(Node* node, osmid nid, const POINT& pos,
                                   const AttrMap& m, StAttrGroups* groups,
                                   const RelMap& nodeRels, const RelLst& rels,
                                   const OsmReadOpts& ops) const;

    static void snapStats(const OsmReadOpts& opts,
                          Graph& g,
                          const BBoxIdx& bbox,
                          size_t gridSize,
                          router::FeedStops& fs,
                          Restrictor& res,
                          const NodeSet& orphanStations);
    static void writeGeoms(Graph& g);
    static void deleteOrphNds(Graph& g);
    static void deleteOrphEdgs(Graph& g, const OsmReadOpts& opts);
    static double dist(const Node* a, const Node* b);
    static double webMercDist(const Node* a, const Node* b);

    static NodeGrid buildNodeIdx(Graph& g, size_t size, const BOX& webMercBox,
                                 bool which);

    static EdgeGrid buildEdgeIdx(Graph& g, size_t size, const BOX& webMercBox);

    static void fixGaps(Graph& g, NodeGrid* ng);
    static void collapseEdges(Graph& g);
    static void writeODirEdgs(Graph& g, Restrictor& restor);
    static void writeSelfEdgs(Graph& g);
    static void writeEdgeTracks(const EdgTracks& tracks);
    static void simplifyGeoms(Graph& g);
    static uint32_t writeComps(Graph& g);
    static bool edgesSim(const Edge* a, const Edge* b);
    static const EdgePL& mergeEdgePL(Edge* a, Edge* b);
    static void getEdgCands(const POINT& s, EdgeCandPQ* ret, EdgeGrid* eg,
                            double d);

    static std::set<Node*> getMatchingNds(const NodePL& s, NodeGrid* ng,
                                          double d);

    static Node* getMatchingNd(const NodePL& s, NodeGrid* ng, double d);

    static NodeSet snapStation(Graph& g, NodePL* s, EdgeGrid* eg, NodeGrid* sng,
                               const OsmReadOpts& opts, Restrictor& restor,
                               bool surHeur, bool orphSnap, double maxD);

    // Checks if from the edge e, a station similar to si can be reach with less
    // than maxD distance and less or equal to "maxFullTurns" full turns. If
    // such a station exists, it is returned. If not, 0 is returned.
    static Node* eqStatReach(const Edge* e, const StatInfo* si, const POINT& p,
                             double maxD, int maxFullTurns, double maxAng,
                             bool orph);

    static Node* depthSearch(const Edge* e, const StatInfo* si, const POINT& p,
                             double maxD, int maxFullTurns, double minAngle,
                             const SearchFunc& sfunc);

    static bool isBlocked(const Edge* e, const StatInfo* si, const POINT& p,
                          double maxD, int maxFullTurns, double minAngle);
    static bool keepFullTurn(const trgraph::Node* n, double ang);

    static StatGroup* groupStats(const NodeSet& s);

    static NodePL plFromGtfs(const Stop* s, const OsmReadOpts& ops);

    std::vector<TransitEdgeLine*> getLines(const std::vector<size_t>& edgeRels,
                                           const RelLst& rels,
                                           const OsmReadOpts& ops);

    void getKeptAttrKeys(const OsmReadOpts& opts, AttrKeySet sets[3]) const;


    void processRestr(osmid nid, osmid wid, const Restrictions& rawRests, Edge* e,
                      Node* n, Restrictor& restor) const;

    std::string getAttrByFirstMatch(const DeepAttrLst& rule, osmid id,
                                    const AttrMap& attrs, const RelMap& entRels,
                                    const RelLst& rels,
                                    const Normalizer& norm) const;

    std::vector<std::string> getAttrMatchRanked(const DeepAttrLst& rule, osmid id,
                                                const AttrMap& attrs,
                                                const RelMap& entRels,
                                                const RelLst& rels,
                                                const Normalizer& norm) const;

    std::string getAttr(const DeepAttrRule& s, osmid id, const AttrMap& attrs,
                        const RelMap& entRels, const RelLst& rels) const;

    bool relKeep(osmid id, const RelMap& rels, const FlatRels& fl) const;

    std::map<TransitEdgeLine, TransitEdgeLine*> _lines;
    std::map<size_t, TransitEdgeLine*> _relLines;
};
}  // namespace pfaedle
#endif  // PFAEDLE_OSM_OSMBUILDER_H_
