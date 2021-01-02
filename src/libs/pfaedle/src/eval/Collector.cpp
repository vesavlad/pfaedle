// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/eval/Collector.h"
#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/Def.h"
#include "pfaedle/eval/Result.h"
#include "util/geo/Geo.h"
#include "util/geo/output/GeoJsonOutput.h"
#include <logging/logger.h>
#include <csignal>
#include <set>
#include <string>
#include <utility>


using ad::cppgtfs::gtfs::Shape;
using pfaedle::eval::Collector;
using pfaedle::eval::Result;
using pfaedle::gtfs::Trip;
using util::geo::output::GeoJsonOutput;

// _____________________________________________________________________________
double Collector::add(const Trip* t, const Shape* oldS, const Shape& newS,
                      const std::vector<double>& newTripDists)
{
    if (!oldS)
    {
        _noOrigShp++;
        return 0;
    }

    for (auto st : t->getStopTimes())
    {
        if (st.getShapeDistanceTravelled() < 0)
        {
            // we cannot safely compare trips without shape dist travelled
            // info
            _noOrigShp++;
            return 0;
        }
    }

    double fd = 0;
    size_t unmatchedSegments;
    double unmatchedSegmentsLength;

    std::vector<double> oldDists;
    LINE oldL = getWebMercLine(
            oldS, t->getStopTimes().begin()->getShapeDistanceTravelled(),
            (--t->getStopTimes().end())->getShapeDistanceTravelled(), &oldDists);

    std::vector<double> newDists;
    LINE newL = getWebMercLine(&newS, -1, -1, &newDists);

    std::ofstream fstr(_evalOutPath + "/trip-" + t->getId() + ".json");
    GeoJsonOutput gjout(fstr);

    auto oldSegs = segmentize(t, oldL, oldDists, nullptr);
    auto newSegs = segmentize(t, newL, newDists, &newTripDists);

    // cut both result at the beginning and end to clear evaluation from
    // loops at the end
    POLYLINE oldStart = oldSegs[0];
    POLYLINE newStart = newSegs[0];
    auto oldStartNew =
            oldStart.getSegment(oldStart.projectOn(newSegs[0][0]).totalPos, 1);
    auto newStartNew =
            newStart.getSegment(newStart.projectOn(oldSegs[0][0]).totalPos, 1);
    if (fabs(oldStartNew.getLength() - oldStart.getLength()) /
                        oldStart.getLength() <
                0.5 &&
        fabs(newStartNew.getLength() - newStart.getLength()) /
                        newStart.getLength() <
                0.5)
    {
        oldSegs[0] = oldStartNew.getLine();
        newSegs[0] = newStartNew.getLine();
    }

    POLYLINE oldEnd = oldSegs[oldSegs.size() - 1];
    POLYLINE newEnd = newSegs[oldSegs.size() - 1];
    auto oldEndNew =
            oldEnd.getSegment(0, oldEnd.projectOn(newSegs.back().back()).totalPos);
    auto newEndNew =
            newEnd.getSegment(0, newEnd.projectOn(oldSegs.back().back()).totalPos);
    if (fabs(oldEndNew.getLength() - oldEnd.getLength()) / oldEnd.getLength() <
                0.5 &&
        fabs(newEndNew.getLength() - newEnd.getLength()) / newEnd.getLength() <
                0.5)
    {
        oldSegs[oldSegs.size() - 1] = oldEndNew.getLine();
        newSegs[newSegs.size() - 1] = newEndNew.getLine();
    }

    // check for suspicious (most likely erroneous) lines in the
    // ground truth data which have a long straight-line segment

    for (auto oldL : oldSegs)
    {
        for (size_t i = 1; i < oldL.size(); i++)
        {
            if (util::geo::webMercMeterDist(oldL[i - 1], oldL[i]) > 500)
            {
                // return 0;
            }
        }
    }

    // new lines build from cleaned-up shapes
    LINE oldLCut;
    LINE newLCut;

    for (auto oldL : oldSegs)
    {
        gjout.printLatLng(oldL, util::json::Dict{{"ver", "old"}});
        oldLCut.insert(oldLCut.end(), oldL.begin(), oldL.end());
    }

    for (auto newL : newSegs)
    {
        gjout.printLatLng(newL, util::json::Dict{{"ver", "new"}});
        newLCut.insert(newLCut.end(), newL.begin(), newL.end());
    }

    gjout.flush();
    fstr.close();

    double fac = cos(2 * atan(exp((oldSegs.front().front().getY() +
                                   oldSegs.back().back().getY()) /
                                  6378137.0)) -
                     1.5707965);

    if (_dCache.count(oldS) && _dCache.find(oldS)->second.count(newS.getId()))
    {
        fd = _dCache[oldS][newS.getId()];
    }
    else
    {
        fd = util::geo::accFrechetDistC(oldLCut, newLCut, 5 / fac) * fac;
        _dCache[oldS][newS.getId()] = fd;
    }

    if (_dACache.count(oldS) && _dACache.find(oldS)->second.count(newS.getId()))
    {
        unmatchedSegments = _dACache[oldS][newS.getId()].first;
        unmatchedSegmentsLength = _dACache[oldS][newS.getId()].second;
    }
    else
    {
        auto dA = getDa(oldSegs, newSegs);
        _dACache[oldS][newS.getId()] = dA;
        unmatchedSegments = dA.first;
        unmatchedSegmentsLength = dA.second;
    }

    double totL = 0;
    for (auto l : oldSegs) totL += util::geo::len(l) * fac;

    // filter out shapes with a lenght of under 5 meters - they are most likely
    // artifacts
    if (totL < 5)
    {
        _noOrigShp++;
        return 0;
    }

    _fdSum += fd / totL;
    _unmatchedSegSum += unmatchedSegments;
    _unmatchedSegLengthSum += unmatchedSegmentsLength;
    _results.insert(Result(t, fd / totL));
    _resultsAN.insert(Result(t, static_cast<double>(unmatchedSegments) /
                                        static_cast<double>(oldSegs.size())));
    _resultsAL.insert(Result(t, unmatchedSegmentsLength / totL));

    LOG(DEBUG) << "This result (" << t->getId()
               << "): A_N/N = " << unmatchedSegments << "/" << oldSegs.size()
               << " = "
               << static_cast<double>(unmatchedSegments) /
                          static_cast<double>(oldSegs.size())
               << " A_L/L = " << unmatchedSegmentsLength << "/" << totL << " = "
               << unmatchedSegmentsLength / totL << " d_f = " << fd;

    return fd;
}

// _____________________________________________________________________________
std::vector<LINE> Collector::segmentize(
        const Trip* t, const LINE& shape, const std::vector<double>& dists,
        const std::vector<double>* newTripDists)
{
    std::vector<LINE> ret;

    if (t->getStopTimes().size() < 2) return ret;

    POLYLINE pl(shape);
    std::vector<std::pair<POINT, double>> cuts;

    size_t i = 0;
    for (auto st : t->getStopTimes())
    {
        if (newTripDists)
        {
            cuts.push_back(std::pair<POINT, double>(
                    util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(st.getStop()->getLat(),
                                                                  st.getStop()->getLng()),
                    (*newTripDists)[i]));
        }
        else
        {
            cuts.push_back(std::pair<POINT, double>(
                    util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(st.getStop()->getLat(),
                                                                  st.getStop()->getLng()),
                    st.getShapeDistanceTravelled()));
        }
        i++;
    }

    // get first half of geometry, and search for start point there!
    size_t before = std::upper_bound(dists.begin(), dists.end(), cuts[1].second) -
                    dists.begin();
    if (before + 1 > shape.size()) before = shape.size() - 1;
    assert(shape.begin() + before + 1 <= shape.end());
    POLYLINE l(LINE(shape.begin(), shape.begin() + before + 1));
    auto lastLp = l.projectOn(cuts.front().first);

    for (size_t i = 1; i < cuts.size(); i++)
    {
        size_t before = shape.size();
        if (i < cuts.size() - 1 && cuts[i + 1].second > -0.5)
        {
            before =
                    std::upper_bound(dists.begin(), dists.end(), cuts[i + 1].second) -
                    dists.begin();
        }

        POLYLINE beforePl(LINE(shape.begin(), shape.begin() + before));

        auto curLp = beforePl.projectOnAfter(cuts[i].first, lastLp.lastIndex);

        ret.push_back(pl.getSegment(lastLp, curLp).getLine());
        lastLp = curLp;
    }

    // std::raise(SIGABRT);
    return ret;
}

// _____________________________________________________________________________
LINE Collector::getWebMercLine(const Shape* s, double from, double t)
{
    return getWebMercLine(s, from, t, nullptr);
}

// _____________________________________________________________________________
LINE Collector::getWebMercLine(const Shape* s, double from, double to,
                               std::vector<double>* dists)
{
    LINE ret;

    auto i = s->getPoints().begin();

    for (; i != s->getPoints().end(); i++)
    {
        auto p = *i;

        if ((from < 0 || (p.travelDist - from) > -0.01))
        {
            if (to >= 0 && (p.travelDist - to) > 0.01) break;

            POINT mercP = util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(p.lat, p.lng);

            ret.push_back(mercP);
            if (dists) dists->push_back(p.travelDist);
        }
    }

    return ret;
}

// _____________________________________________________________________________
const std::set<Result>& Collector::getResults() const { return _results; }

// _____________________________________________________________________________
double Collector::getAvgDist() const { return _fdSum / _results.size(); }

// _____________________________________________________________________________
void Collector::printHisto(std::ostream* os, const std::set<Result>& result,
                           const std::vector<double>& bins) const
{
    size_t W = 60;

    auto it = result.begin();
    std::vector<std::pair<double, size_t>> res;
    std::vector<const Trip*> examples;
    size_t maxC = 0;

    for (double bin : bins)
    {
        size_t c = 0;
        const Trip* trip = nullptr;

        while (it != result.end() && it->getDist() <= (bin + 0.001))
        {
            if (!trip) trip = it->getTrip();
            c++;
            it++;
        }

        if (c > maxC) maxC = c;

        examples.push_back(trip);
        res.push_back(std::pair<double, size_t>(bin, c));
    }

    size_t j = 0;
    for (auto r : res)
    {
        std::string range = util::toString(r.first);
        (*os) << "  < " << std::setfill(' ') << std::setw(10) << range << ": ";
        size_t i = 0;

        for (; i < W * (static_cast<double>(r.second) / static_cast<double>(maxC));
             i++)
        {
            (*os) << "|";
        }

        if (r.second)
            (*os) << " (" << r.second << ", e.g. #" << examples[j]->getId() << ")";
        (*os) << std::endl;
        j++;
    }
}

// _____________________________________________________________________________
std::vector<double> Collector::getBins(double mind, double maxd, size_t steps)
{
    double bin = (maxd - mind) / steps;
    double curE = mind + bin;

    std::vector<double> ret;
    while (curE <= maxd)
    {
        ret.push_back(curE);
        curE += bin;
    }
    return ret;
}

// _____________________________________________________________________________
void Collector::printCsv(std::ostream* os, const std::set<Result>& result,
                         const std::vector<double>& bins) const
{
    auto it = result.begin();
    std::vector<std::pair<double, size_t>> res;

    for (double bin : bins)
    {
        size_t c = 0;
        const Trip* trip = nullptr;

        while (it != result.end() && it->getDist() <= (bin + 0.001))
        {
            if (!trip) trip = it->getTrip();
            c++;
            it++;
        }

        res.push_back(std::pair<double, size_t>(bin, c));
    }

    (*os) << "range, count\n";
    for (auto r : res)
    {
        (*os) << r.first << "," << r.second << "\n";
    }
}

// _____________________________________________________________________________
void Collector::printStats(std::ostream* os) const
{
    size_t buckets = 10;
    (*os) << "\n ===== Evalution results =====\n\n";

    (*os) << std::setfill(' ') << std::setw(30)
          << "  # of trips new shapes were matched for: " << _results.size()
          << "\n";
    (*os) << std::setw(30) << "  # of trips without input shapes: " << _noOrigShp
          << "\n";

    if (!_results.empty())
    {
        (*os) << std::setw(30) << "  highest distance to input shapes: "
              << (--_results.end())->getDist() << " (on trip #"
              << (--_results.end())->getTrip()->getId() << ")\n";
        (*os) << std::setw(30) << "  lowest distance to input shapes: "
              << (_results.begin())->getDist() << " (on trip #"
              << (_results.begin())->getTrip()->getId() << ")\n";
        (*os) << std::setw(30) << "  avg total frechet distance: " << getAvgDist()
              << "\n";

        std::vector<double> dfBins = getBins(
                (_results.begin())->getDist(), (--_results.end())->getDist(), buckets);

        if (!_dfBins.empty()) dfBins = _dfBins;

        (*os) << "\n  -- Histogram of d_f for this run -- " << std::endl;
        printHisto(os, _results, dfBins);

        std::ofstream fstr1(_evalOutPath + "/eval-frechet.csv");
        printCsv(&fstr1, _results, dfBins);

        (*os) << "\n\n\n  -- Histogram of A_N/N for this run -- " << std::endl;
        printHisto(os, _resultsAN,
                   getBins((_resultsAN.begin())->getDist(),
                           (--_resultsAN.end())->getDist(), buckets));
        std::ofstream fstr2(_evalOutPath + "/eval-AN.csv");
        printCsv(&fstr2, _resultsAN, getBins(0, 1, 20));

        (*os) << "\n\n\n  -- Histogram of A_L/L for this run -- " << std::endl;
        printHisto(os, _resultsAL,
                   getBins((_resultsAL.begin())->getDist(),
                           (--_resultsAL.end())->getDist(), buckets));
        std::ofstream fstr3(_evalOutPath + "/eval-AL.csv");
        printCsv(&fstr3, _resultsAL, getBins(0, 1, 20));
    }

    (*os) << "\n ===== End of evaluation results =====\n";
    (*os) << std::endl;
}

// _____________________________________________________________________________
std::pair<size_t, double> Collector::getDa(const std::vector<LINE>& a,
                                           const std::vector<LINE>& b)
{
    assert(a.size() == b.size());
    std::pair<size_t, double> ret{0, 0};

    // euclidean distance on web mercator is in meters on equator,
    // and proportional to cos(lat) in both y directions
    double fac =
            cos(2 * atan(exp((a.front().front().getY() + a.back().back().getY()) /
                             6378137.0)) -
                1.5707965);

    for (size_t i = 0; i < a.size(); i++)
    {
        double fd = util::geo::frechetDist(a[i], b[i], 3 / fac) * fac;
        if (fd >= 20)
        {
            ret.first++;
            ret.second += util::geo::len(a[i]) * fac;
        }
    }

    return ret;
}
