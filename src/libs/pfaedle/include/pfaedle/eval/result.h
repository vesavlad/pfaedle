// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_EVAL_RESULT_H_
#define PFAEDLE_EVAL_RESULT_H_

namespace pfaedle::gtfs
{
struct trip;
}
namespace pfaedle::eval
{

/*
 * A single evaluation result.
 */
class result
{
public:
    result(const gtfs::trip& t, double dist) :
        _t(t),
        _dist(dist)
    {}

    double get_dist() const
    {
        return _dist;
    }

    const gtfs::trip& get_trip() const
    {
        return _t;
    }

private:
    const gtfs::trip& _t;
    double _dist;
};

inline bool operator<(const result& lhs, const result& rhs)
{
    return lhs.get_dist() < rhs.get_dist() ||
           (lhs.get_dist() == rhs.get_dist() && &(lhs.get_trip()) < &(rhs.get_trip()));
}

}  // namespace pfaedle

#endif  // PFAEDLE_EVAL_RESULT_H_
