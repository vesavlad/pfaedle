// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GEO_LINE_H_
#define UTIL_GEO_LINE_H_

#include <util/geo/Point.h>
#include <vector>

namespace util::geo
{

template<typename T>
class Line : public std::vector<Point<T>>
{
    using std::vector<Point<T>>::vector;
};

template<typename T>
using LineSegment = std::pair<Point<T>, Point<T>>;

template<typename T>
using MultiLine = std::vector<Line<T>>;

}  // namespace util

#endif  // UTIL_GEO_LINE_H_
