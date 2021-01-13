// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_EVAL_COLLECTOR_H_
#define PFAEDLE_EVAL_COLLECTOR_H_

#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/Def.h"
#include "pfaedle/eval/result.h"
#include "pfaedle/gtfs/Feed.h"
#include "util/geo/Geo.h"
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

using pfaedle::gtfs::Trip;
using ad::cppgtfs::gtfs::Shape;

namespace pfaedle::eval
{

/*
 * Collects routing results for evaluation
 */
class collector
{
public:
    collector(const std::string& evalOutPath, const std::vector<double>& dfBins);

    // Add a shape found by our tool newS for a trip t with newly calculated
    // station dist values with the old shape oldS
    double add(const Trip& t, const Shape* oldS, const Shape& newS, const std::vector<double>& newDists);

    // Return the set of all Result objects
    const std::set<result>& get_results() const;

    // Print general stats to os
    void print_stats(std::ostream& os) const;

    // Print histograms for the results to os
    void print_histogram(std::ostream& os, const std::set<result>& result, const std::vector<double>& bins) const;

    // Print a CSV for the results to os
    void print_csv(std::ostream& os, const std::set<result>& result, const std::vector<double>& bins) const;

    // Return the averaged average frechet distance
    double get_average_distance() const;

    static LINE get_web_merc_line(const Shape* s, double from, double to);
    static LINE get_web_merc_line(const Shape* s, double from, double to, std::vector<double>& dists);

private:
    static std::pair<size_t, double> get_da(const std::vector<LINE>& a,
                                           const std::vector<LINE>& b);

    static std::vector<LINE> segmentize(const Trip& t, const LINE& shape,
                                        const std::vector<double>& dists,
                                        const std::vector<double>* newTripDists);

    static std::vector<double> get_bins(double mind, double maxd, size_t steps);

    std::set<result> _results;
    std::set<result> _resultsAN;
    std::set<result> _resultsAL;
    std::map<const Shape*, std::map<std::string, double>> _dCache;
    std::map<const Shape*, std::map<std::string, std::pair<size_t, double>>> _dACache;
    size_t _noOrigShp;

    double _fdSum;
    size_t _unmatchedSegSum;
    double _unmatchedSegLengthSum;

    std::string _evalOutPath;

    std::vector<double> _dfBins;
};

}  // namespace pfaedle

#endif  // PFAEDLE_EVAL_COLLECTOR_H_
