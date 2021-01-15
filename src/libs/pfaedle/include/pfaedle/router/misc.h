// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_MISC_H_
#define PFAEDLE_ROUTER_MISC_H_

#include <gtfs/feed.h>
#include <gtfs/route.h>
#include <gtfs/stop.h>
#include <gtfs/route_type.h>
#include <pfaedle/trgraph/graph.h>
#include <util/geo/Point.h>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace pfaedle::router
{

struct node_candidate
{
    trgraph::node* nd;
    double pen;
};

struct edge_candidate
{
    trgraph::edge* e;
    double pen;
};

struct routing_options
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

inline bool operator==(const routing_options& a, const routing_options& b)
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

struct edge_cost
{
    edge_cost() :
        _cost(0)
    {}

    explicit edge_cost(double cost) :
        _cost(cost)
    {}

    edge_cost(double mDist, double mDistLvl1, double mDistLvl2, double mDistLvl3,
             double mDistLvl4, double mDistLvl5, double mDistLvl6,
             double mDistLvl7, uint32_t fullTurns, int32_t passThru,
             double oneWayMeters, size_t oneWayEdges, double lineUnmatchedMeters,
             double noLinesMeters, double reachPen, const routing_options* o)
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

inline edge_cost operator+(const edge_cost& a, const edge_cost& b)
{
    return edge_cost(a.getValue() + b.getValue());
}

inline bool operator<=(const edge_cost& a, const edge_cost& b)
{
    return a.getValue() <= b.getValue();
}

inline bool operator==(const edge_cost& a, const edge_cost& b)
{
    return a.getValue() == b.getValue();
}

inline bool operator>(const edge_cost& a, const edge_cost& b)
{
    return a.getValue() > b.getValue();
}

template<typename F>
inline bool angSmaller(const util::geo::Point<F>& f,
                       const util::geo::Point<F>& m,
                       const util::geo::Point<F>& t,
                       double ang)
{
    if (util::geo::innerProd(m, f, t) < ang)
    {
        return true;
    }

    return false;
}

using node_set = std::set<trgraph::node*>;
using edge_set = std::set<trgraph::edge*>;
using feed_stops = std::unordered_map<const pfaedle::gtfs::stop*, trgraph::node*>;

using node_candidate_group = std::vector<node_candidate>;
using node_candidate_route = std::vector<node_candidate_group>;

using edge_candidate_group = std::vector<edge_candidate>;
using edge_candidate_route = std::vector<edge_candidate_group>;

using edge_list = std::vector<trgraph::edge*>;
using node_list = std::vector<trgraph::node*>;

struct edge_list_hop
{
    edge_list edges;
    const trgraph::node* start;
    const trgraph::node* end;
};

using edge_list_hops = std::vector<edge_list_hop>;

using route_type_set = std::set<pfaedle::gtfs::route_type>;

route_type_set route_type_section(const route_type_set& a, const route_type_set& b);

pfaedle::router::feed_stops write_mot_stops(const pfaedle::gtfs::feed& feed,
                                                const route_type_set& mots,
                                                const std::string& tid);

std::string get_mot_str(const route_type_set& mots);

}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_MISC_H_
