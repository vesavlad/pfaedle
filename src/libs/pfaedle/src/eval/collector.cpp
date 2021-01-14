// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/eval/collector.h"
#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/definitions.h"
#include "pfaedle/eval/result.h"
#include "util/geo/Geo.h"
#include "util/geo/output/GeoJsonOutput.h"

#include <logging/logger.h>
#include <cmath>
#include <csignal>
#include <set>
#include <string>
#include <utility>


using ad::cppgtfs::gtfs::Shape;
using pfaedle::gtfs::Trip;

namespace pfaedle::eval
{

collector::collector(const std::string& evalOutPath, const std::vector<double>& dfBins) :
    _noOrigShp(0),
    _fdSum(0),
    _unmatchedSegSum(0),
    _unmatchedSegLengthSum(0),
    _evalOutPath(evalOutPath),
    _dfBins(dfBins)
{}

double collector::add(const Trip& t,
                      const Shape* oldS,
                      const Shape& newS,
                      const std::vector<double>& newTripDists)
{
    if (!oldS)
    {
        _noOrigShp++;
        return 0;
    }

    for (auto st : t.getStopTimes())
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
    size_t unmatched_segments = 0;
    double unmatched_segments_length = NAN;

    std::vector<double> old_dists;
    LINE old_line = get_web_merc_line(
            oldS, t.getStopTimes().begin()->getShapeDistanceTravelled(),
            (--t.getStopTimes().end())->getShapeDistanceTravelled(), old_dists);

    std::vector<double> new_dists;
    LINE new_line = get_web_merc_line(&newS, -1, -1, new_dists);

    std::ofstream fstr(_evalOutPath + "/trip-" + t.getId() + ".json");
    util::geo::output::GeoJsonOutput gjout(fstr);

    auto old_segs = segmentize(t, old_line, old_dists, nullptr);
    auto new_segs = segmentize(t, new_line, new_dists, &newTripDists);

    // cut both result at the beginning and end to clear evaluation from
    // loops at the end
    POLYLINE old_start = old_segs[0];
    POLYLINE new_start = new_segs[0];
    auto old_start_new = old_start.getSegment(old_start.projectOn(new_segs[0][0]).totalPos, 1);
    auto new_start_new = new_start.getSegment(new_start.projectOn(old_segs[0][0]).totalPos, 1);

    if (fabs(old_start_new.getLength() - old_start.getLength()) / old_start.getLength() < 0.5 &&
        fabs(new_start_new.getLength() - new_start.getLength()) / new_start.getLength() < 0.5)
    {
        old_segs[0] = old_start_new.getLine();
        new_segs[0] = new_start_new.getLine();
    }

    POLYLINE old_end = old_segs[old_segs.size() - 1];
    POLYLINE new_end = new_segs[old_segs.size() - 1];
    auto old_end_new = old_end.getSegment(0, old_end.projectOn(new_segs.back().back()).totalPos);
    auto new_end_new = new_end.getSegment(0, new_end.projectOn(old_segs.back().back()).totalPos);

    if (fabs(old_end_new.getLength() - old_end.getLength()) / old_end.getLength() < 0.5 &&
        fabs(new_end_new.getLength() - new_end.getLength()) / new_end.getLength() < 0.5)
    {
        old_segs[old_segs.size() - 1] = old_end_new.getLine();
        new_segs[new_segs.size() - 1] = new_end_new.getLine();
    }

    // check for suspicious (most likely erroneous) lines in the
    // ground truth data which have a long straight-line segment

    for (const auto& old_line_segs : old_segs)
    {
        for (size_t i = 1; i < old_line_segs.size(); i++)
        {
            if (util::geo::webMercMeterDist(old_line_segs[i - 1], old_line_segs[i]) > 500)
            {
                // return 0;
            }
        }
    }

    // new lines build from cleaned-up shapes
    LINE old_l_cut;
    LINE new_l_cut;

    for (const auto& old_line_seg : old_segs)
    {
        gjout.printLatLng(old_line_seg, util::json::Dict{{"ver", "old"}});
        old_l_cut.insert(old_l_cut.end(), old_line_seg.begin(), old_line_seg.end());
    }

    for (const auto& new_line_seg : new_segs)
    {
        gjout.printLatLng(new_line_seg, util::json::Dict{{"ver", "new"}});
        new_l_cut.insert(new_l_cut.end(), new_line_seg.begin(), new_line_seg.end());
    }

    gjout.flush();
    fstr.close();

    double fac = cos(2 * atan(exp((old_segs.front().front().getY() +
                                   old_segs.back().back().getY()) /
                                  6378137.0)) -
                     1.5707965);

    if (_dCache.count(oldS) && _dCache.find(oldS)->second.count(newS.getId()))
    {
        fd = _dCache[oldS][newS.getId()];
    }
    else
    {
        fd = util::geo::accFrechetDistC(old_l_cut, new_l_cut, 5 / fac) * fac;
        _dCache[oldS][newS.getId()] = fd;
    }

    if (_dACache.count(oldS) && _dACache.find(oldS)->second.count(newS.getId()))
    {
        unmatched_segments = _dACache[oldS][newS.getId()].first;
        unmatched_segments_length = _dACache[oldS][newS.getId()].second;
    }
    else
    {
        auto dA = get_da(old_segs, new_segs);
        _dACache[oldS][newS.getId()] = dA;
        unmatched_segments = dA.first;
        unmatched_segments_length = dA.second;
    }

    double total_length = 0;
    for (const auto& l : old_segs)
        total_length += util::geo::len(l) * fac;

    // filter out shapes with a length of under 5 meters - they are most likely
    // artifacts
    if (total_length < 5)
    {
        _noOrigShp++;
        return 0;
    }

    _fdSum += fd / total_length;
    _unmatchedSegSum += unmatched_segments;
    _unmatchedSegLengthSum += unmatched_segments_length;
    _results.insert(result(t, fd / total_length));
    _resultsAN.insert(result(t, static_cast<double>(unmatched_segments) / static_cast<double>(old_segs.size())));
    _resultsAL.insert(result(t, unmatched_segments_length / total_length));

    LOG(DEBUG) << "This result (" << t.getId()
               << "): A_N/N = " << unmatched_segments << "/" << old_segs.size()
               << " = "
               << static_cast<double>(unmatched_segments) /
                          static_cast<double>(old_segs.size())
               << " A_L/L = " << unmatched_segments_length << "/" << total_length << " = "
               << unmatched_segments_length / total_length << " d_f = " << fd;

    return fd;
}

std::vector<LINE> collector::segmentize(
        const Trip& t,
        const LINE& shape,
        const std::vector<double>& dists,
        const std::vector<double>* new_trip_dists)
{
    std::vector<LINE> ret;

    if (t.getStopTimes().size() < 2) return ret;

    POLYLINE pl(shape);
    std::vector<std::pair<POINT, double>> cuts;

    size_t i = 0;
    for (auto st : t.getStopTimes())
    {
        if (new_trip_dists)
        {
            cuts.emplace_back(
                    util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(st.getStop()->getLat(), st.getStop()->getLng()),
                    (*new_trip_dists)[i]);
        }
        else
        {
            cuts.emplace_back(
                    util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(st.getStop()->getLat(), st.getStop()->getLng()),
                    st.getShapeDistanceTravelled());
        }
        i++;
    }

    // get first half of geometry, and search for start point there!
    size_t before = std::upper_bound(dists.begin(), dists.end(), cuts[1].second) - dists.begin();
    if ((before + 1) > shape.size())
    {
        before = shape.size() - 1;
    }

    assert(shape.begin() + before + 1 <= shape.end());

    POLYLINE l(LINE(shape.begin(), shape.begin() + before + 1));
    auto last_lp = l.projectOn(cuts.front().first);

    for (size_t c_i = 1; c_i < cuts.size(); c_i++)
    {
        size_t size_before = shape.size();
        if (c_i < cuts.size() - 1 && cuts[c_i + 1].second > -0.5)
        {
            size_before = std::upper_bound(dists.begin(), dists.end(), cuts[c_i + 1].second) - dists.begin();
        }

        POLYLINE before_pl(LINE(shape.begin(), shape.begin() + size_before));

        auto cur_lp = before_pl.projectOnAfter(cuts[c_i].first, last_lp.lastIndex);

        ret.push_back(pl.getSegment(last_lp, cur_lp).getLine());
        last_lp = cur_lp;
    }

    // std::raise(SIGABRT);
    return ret;
}

LINE collector::get_web_merc_line(const Shape* s, double from, double to)
{
    std::vector<double> distances;
    return get_web_merc_line(s, from, to, distances);
}

LINE collector::get_web_merc_line(const Shape* s, double from, double to, std::vector<double>& dists)
{
    LINE ret;

    auto i = s->getPoints().begin();

    for (; i != s->getPoints().end(); i++)
    {
        auto p = *i;

        if ((from < 0 || (p.travelDist - from) > -0.01))
        {
            if (to >= 0 && (p.travelDist - to) > 0.01) break;

            POINT merc_p = util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(p.lat, p.lng);

            ret.push_back(merc_p);
            dists.push_back(p.travelDist);
        }
    }

    return ret;
}

const std::set<result>& collector::get_results() const { return _results; }

double collector::get_average_distance() const { return _fdSum / _results.size(); }

void collector::print_histogram(std::ostream& os, const std::set<result>& result, const std::vector<double>& bins) const
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

        while (it != result.end() && it->get_dist() <= (bin + 0.001))
        {
            if (!trip)
                trip = &it->get_trip();
            c++;
            it++;
        }

        if (c > maxC) maxC = c;

        examples.push_back(trip);
        res.emplace_back(bin, c);
    }

    size_t j = 0;
    for (auto r : res)
    {
        std::string range = util::toString(r.first);
        os << "  < " << std::setfill(' ') << std::setw(10) << range << ": ";
        size_t i = 0;

        for (; i < W * (static_cast<double>(r.second) / static_cast<double>(maxC));
             i++)
        {
            os << "|";
        }

        if (r.second)
            os << " (" << r.second << ", e.g. #" << examples[j]->getId() << ")";
        os << std::endl;
        j++;
    }
}

std::vector<double> collector::get_bins(double mind, double maxd, size_t steps)
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

void collector::print_csv(std::ostream& os,
                         const std::set<result>& result,
                         const std::vector<double>& bins) const
{
    std::vector<std::pair<double, size_t>> res;

    auto it = result.begin();
    for (double bin : bins)
    {
        size_t c = 0;
        const Trip* trip = nullptr;

        while (it != result.end() && (it->get_dist() <= (bin + 0.001)))
        {
            if (!trip) {
                trip = &it->get_trip();
            }

            c++;
            it++;
        }

        res.emplace_back(bin, c);
    }

    os << "range, count\n";
    for (auto r : res)
    {
        os << r.first << "," << r.second << "\n";
    }
}

void collector::print_stats(std::ostream& os) const
{
    size_t buckets = 10;
    os << "\n ===== Evalution results =====\n\n";

    os << std::setfill(' ') << std::setw(30)
       << "  # of trips new shapes were matched for: " << _results.size()
       << "\n";
    os << std::setw(30) << "  # of trips without input shapes: " << _noOrigShp
       << "\n";

    if (!_results.empty())
    {
        os << std::setw(30) << "  highest distance to input shapes: "
           << (--_results.end())->get_dist() << " (on trip #"
           << (--_results.end())->get_trip().getId() << ")\n";
        os << std::setw(30) << "  lowest distance to input shapes: "
           << (_results.begin())->get_dist() << " (on trip #"
           << (_results.begin())->get_trip().getId() << ")\n";
        os << std::setw(30) << "  avg total frechet distance: " << get_average_distance()
           << "\n";

        std::vector<double> dfBins = get_bins(
                (_results.begin())->get_dist(), (--_results.end())->get_dist(), buckets);

        if (!_dfBins.empty()) dfBins = _dfBins;

        os << "\n  -- Histogram of d_f for this run -- " << std::endl;
        print_histogram(os, _results, dfBins);

        std::ofstream fstr1(_evalOutPath + "/eval-frechet.csv");
        print_csv(fstr1, _results, dfBins);

        os << "\n\n\n  -- Histogram of A_N/N for this run -- " << std::endl;
        print_histogram(os, _resultsAN,
                        get_bins((_resultsAN.begin())->get_dist(),
                                 (--_resultsAN.end())->get_dist(), buckets));
        std::ofstream fstr2(_evalOutPath + "/eval-AN.csv");
        print_csv(fstr2, _resultsAN, get_bins(0, 1, 20));

        os << "\n\n\n  -- Histogram of A_L/L for this run -- " << std::endl;
        print_histogram(os, _resultsAL,
                        get_bins((_resultsAL.begin())->get_dist(),
                                 (--_resultsAL.end())->get_dist(), buckets));
        std::ofstream fstr3(_evalOutPath + "/eval-AL.csv");
        print_csv(fstr3, _resultsAL, get_bins(0, 1, 20));
    }

    os << "\n ===== End of evaluation results =====\n";
    os << std::endl;
}

std::pair<size_t, double> collector::get_da(const std::vector<LINE>& a,
                                           const std::vector<LINE>& b)
{
    assert(a.size() == b.size());
    std::pair<size_t, double> ret{0, 0};

    // euclidean distance on web mercator is in meters on equator,
    // and proportional to cos(lat) in both y directions
    double fac = cos(2 * atan(exp((a.front().front().getY() + a.back().back().getY()) / 6378137.0)) - 1.5707965);

    for (size_t i = 0; i < a.size(); i++)
    {
        double fd = util::geo::frechet_distance(a[i], b[i], 3 / fac) * fac;
        if (fd >= 20)
        {
            ret.first++;
            ret.second += util::geo::len(a[i]) * fac;
        }
    }

    return ret;
}

}
