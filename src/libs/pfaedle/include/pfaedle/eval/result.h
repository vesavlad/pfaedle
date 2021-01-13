// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_EVAL_RESULT_H_
#define PFAEDLE_EVAL_RESULT_H_

#include "pfaedle/gtfs/Feed.h"
#include "cppgtfs/gtfs/Feed.h"

using pfaedle::gtfs::Trip;
using ad::cppgtfs::gtfs::Shape;

namespace pfaedle::eval
{

/*
 * A single evaluation result.
 */
class result
{
public:
    result(const Trip& t, double dist) :
        _t(t),
        _dist(dist)
    {}

    double get_dist() const
    {
        return _dist;
    }

    const Trip& get_trip() const
    {
        return _t;
    }

private:
    const Trip& _t;
    double _dist;
};

inline bool operator<(const result& lhs, const result& rhs)
{
    return lhs.get_dist() < rhs.get_dist() ||
           (lhs.get_dist() == rhs.get_dist() && &(lhs.get_trip()) < &(rhs.get_trip()));
}

}  // namespace pfaedle

#endif  // PFAEDLE_EVAL_RESULT_H_
