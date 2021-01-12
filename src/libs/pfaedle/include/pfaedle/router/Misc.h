// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_MISC_H_
#define PFAEDLE_ROUTER_MISC_H_

#include <cppgtfs/gtfs/Feed.h>
#include <cppgtfs/gtfs/Stop.h>
#include <cppgtfs/gtfs/Route.h>
#include <pfaedle/gtfs/Feed.h>
#include <pfaedle/trgraph/Graph.h>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace pfaedle::router
{

struct NodeCandidate
{
    trgraph::Node* nd;
    double pen;
};

struct EdgeCandidate
{
    trgraph::Edge* e;
    double pen;
};

struct RoutingOptions
{
    double fullTurnPunishFac{2000};
    double fullTurnAngle{45};
    double passThruStationsPunish{100};
    double oneWayPunishFac{1};
    double oneWayEdgePunish{0};
    double lineUnmatchedPunishFact{0.5};
    double noLinesPunishFact{0};
    double platformUnmatchedPen{0};
    double stationDistPenFactor{0};
    double nonOsmPen;
    double levelPunish[8];
    bool popReachEdge{true};
    bool noSelfHops{true};
};

inline bool operator==(const RoutingOptions& a, const RoutingOptions& b)
{
    return fabs(a.fullTurnPunishFac - b.fullTurnPunishFac) < 0.01 &&
           fabs(a.fullTurnAngle - b.fullTurnAngle) < 0.01 &&
           fabs(a.passThruStationsPunish - b.passThruStationsPunish) < 0.01 &&
           fabs(a.oneWayPunishFac - b.oneWayPunishFac) < 0.01 &&
           fabs(a.oneWayEdgePunish - b.oneWayEdgePunish) < 0.01 &&
           fabs(a.lineUnmatchedPunishFact - b.lineUnmatchedPunishFact) < 0.01 &&
           fabs(a.noLinesPunishFact - b.noLinesPunishFact) < 0.01 &&
           fabs(a.platformUnmatchedPen - b.platformUnmatchedPen) < 0.01 &&
           fabs(a.stationDistPenFactor - b.stationDistPenFactor) < 0.01 &&
           fabs(a.nonOsmPen - b.nonOsmPen) < 0.01 &&
           fabs(a.levelPunish[0] - b.levelPunish[0]) < 0.01 &&
           fabs(a.levelPunish[1] - b.levelPunish[1]) < 0.01 &&
           fabs(a.levelPunish[2] - b.levelPunish[2]) < 0.01 &&
           fabs(a.levelPunish[3] - b.levelPunish[3]) < 0.01 &&
           fabs(a.levelPunish[4] - b.levelPunish[4]) < 0.01 &&
           fabs(a.levelPunish[5] - b.levelPunish[5]) < 0.01 &&
           fabs(a.levelPunish[6] - b.levelPunish[6]) < 0.01 &&
           fabs(a.levelPunish[7] - b.levelPunish[7]) < 0.01 &&
           a.popReachEdge == b.popReachEdge && a.noSelfHops == b.noSelfHops;
}

struct EdgeCost
{
    EdgeCost() :
        _cost(0)
    {}

    explicit EdgeCost(double cost) :
        _cost(cost)
    {}

    EdgeCost(double mDist, double mDistLvl1, double mDistLvl2, double mDistLvl3,
             double mDistLvl4, double mDistLvl5, double mDistLvl6,
             double mDistLvl7, uint32_t fullTurns, int32_t passThru,
             double oneWayMeters, size_t oneWayEdges, double lineUnmatchedMeters,
             double noLinesMeters, double reachPen, const RoutingOptions* o)
    {
        if (!o)
        {
            _cost = mDist + reachPen;
        }
        else
        {
            _cost = mDist * o->levelPunish[0] + mDistLvl1 * o->levelPunish[1] +
                    mDistLvl2 * o->levelPunish[2] + mDistLvl3 * o->levelPunish[3] +
                    mDistLvl4 * o->levelPunish[4] + mDistLvl5 * o->levelPunish[5] +
                    mDistLvl6 * o->levelPunish[6] + mDistLvl7 * o->levelPunish[7] +
                    oneWayMeters * o->oneWayPunishFac +
                    oneWayEdges * o->oneWayEdgePunish +
                    lineUnmatchedMeters * o->lineUnmatchedPunishFact +
                    noLinesMeters * o->noLinesPunishFact +
                    fullTurns * o->fullTurnPunishFac +
                    passThru * o->passThruStationsPunish + reachPen;
        }
    }

    double getValue() const
    {
        return _cost;
    }

private:
    double _cost;
};

inline EdgeCost operator+(const EdgeCost& a, const EdgeCost& b)
{
    return EdgeCost(a.getValue() + b.getValue());
}

inline bool operator<=(const EdgeCost& a, const EdgeCost& b)
{
    return a.getValue() <= b.getValue();
}

inline bool operator==(const EdgeCost& a, const EdgeCost& b)
{
    return a.getValue() == b.getValue();
}

inline bool operator>(const EdgeCost& a, const EdgeCost& b)
{
    return a.getValue() > b.getValue();
}

template<typename F>
inline bool angSmaller(const Point<F>& f, const Point<F>& m, const Point<F>& t,
                       double ang)
{
    if (util::geo::innerProd(m, f, t) < ang)
    {
        return true;
    }

    return false;
}

using NodeSet = std::set<trgraph::Node*>;
using EdgeSet = std::set<trgraph::Edge*>;
using FeedStops = std::unordered_map<const ad::cppgtfs::gtfs::Stop*, trgraph::Node*>;

using NodeCandidateGroup = std::vector<NodeCandidate>;
using NodeCandidateRoute = std::vector<NodeCandidateGroup>;

using EdgeCandidateGroup = std::vector<EdgeCandidate>;
using EdgeCandidateRoute = std::vector<EdgeCandidateGroup>;

using EdgeList = std::vector<trgraph::Edge*>;
using NodeList = std::vector<trgraph::Node*>;

struct EdgeListHop
{
    EdgeList edges;
    const trgraph::Node* start;
    const trgraph::Node* end;
};

using EdgeListHops = std::vector<EdgeListHop>;

using MOTs = std::set<ad::cppgtfs::gtfs::Route::TYPE>;

inline MOTs motISect(const MOTs& a, const MOTs& b)
{
    MOTs ret;
    for (auto mot : a)
    {
        if (b.count(mot))
        {
            ret.insert(mot);
        }
    }
    return ret;
}

inline pfaedle::router::FeedStops writeMotStops(const pfaedle::gtfs::Feed& feed,
                                                const MOTs& mots,
                                                const std::string& tid)
{
    pfaedle::router::FeedStops ret;
    for (auto t : feed.getTrips())
    {
        if (!tid.empty() && t.getId() != tid)
            continue;

        if (mots.count(t.getRoute()->getType()))
        {
            for (auto st : t.getStopTimes())
            {
                // if the station has type STATION_ENTRANCE, use the parent
                // station for routing. Normally, this should not occur, as
                // this is not allowed in stop_times.txt
                if (st.getStop()->getLocationType() == ad::cppgtfs::gtfs::flat::Stop::STATION_ENTRANCE &&
                    st.getStop()->getParentStation())
                {
                    ret[st.getStop()->getParentStation()] = nullptr;
                }
                else
                {
                    ret[st.getStop()] = nullptr;
                }
            }
        }
    }
    return ret;
}

inline std::string getMotStr(const MOTs& mots)
{
    bool first = false;
    std::string motStr;
    for (const auto& mot : mots)
    {
        if (first)
        {
            motStr += ", ";
        }
        motStr += "<" + ad::cppgtfs::gtfs::flat::Route::getTypeString(mot) + ">";
        first = true;
    }

    return motStr;
}
}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_MISC_H_
