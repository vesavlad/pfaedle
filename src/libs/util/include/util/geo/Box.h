// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GEO_BOX_H_
#define UTIL_GEO_BOX_H_

#include "util/geo/Point.h"

namespace util::geo
{

template<typename T>
class Box
{
public:
    // maximum inverse box as default value of box
    Box() :
        lower_left_(std::numeric_limits<T>::max(), std::numeric_limits<T>::max()),
        upper_right_(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest())
    {}

    Box(const Point<T>& ll, const Point<T>& ur) :
        lower_left_(ll),
        upper_right_(ur)
    {}

    const Point<T>& getLowerLeft() const { return lower_left_; }
    const Point<T>& getUpperRight() const { return upper_right_; }

    Point<T>& getLowerLeft() { return lower_left_; }
    Point<T>& getUpperRight() { return upper_right_; }

    void setLowerLeft(const Point<T>& ll) { lower_left_ = ll; }
    void setUpperRight(const Point<T>& ur) { upper_right_ = ur; }

    bool operator==(const Box<T>& b) const
    {
        return getLowerLeft() == b.getLowerLeft() &&
               getUpperRight() == b.getUpperRight();
    }

    bool operator!=(const Box<T>& p) const { return !(*this == p); }

private:
    Point<T> lower_left_;
    Point<T> upper_right_;
};

template<typename T>
class RotatedBox
{
public:
    RotatedBox() :
        _box(), _deg(0), _center()
    {}

    RotatedBox(const Box<T>& box) :
        _box(box),
        _deg(0),
        _center(Point<T>(
                (box.getUpperRight().getX() - box.getLowerLeft().getX()) / T(2),
                (box.getUpperRight().getY() - box.getLowerLeft().getY()) / T(2)))
    {}

    RotatedBox(const Point<T>& ll, const Point<T>& ur) :
        _box(ll, ur),
        _deg(0),
        _center(Point<T>((ur.getX() - ll.getX()) / T(2),
                         (ur.getY() - ll.getY()) / T(2)))
    {}

    RotatedBox(const Box<T>& box, double deg) :
        _box(box),
        _deg(deg),
        _center(Point<T>(
                (box.getUpperRight().getX() - box.getLowerLeft().getX()) / T(2),
                (box.getUpperRight().getY() - box.getLowerLeft().getY()) / T(2)))
    {}

    RotatedBox(const Point<T>& ll, const Point<T>& ur, double deg) :
        _box(ll, ur),
        _deg(deg),
        _center(Point<T>((ur.getX() - ll.getX()) / T(2),
                         (ur.getY() - ll.getY()) / T(2)))
    {}

    RotatedBox(const Box<T>& box, double deg, const Point<T>& center) :
        _box(box), _deg(deg), _center(center)
    {}

    RotatedBox(const Point<T>& ll, const Point<T>& ur, double deg,
               const Point<T>& center) :
        _box(ll, ur),
        _deg(deg), _center(center)
    {}

    const Box<T>& getBox() const { return _box; }
    Box<T>& getBox() { return _box; }

    double getDegree() const { return _deg; }
    const Point<T>& getCenter() const { return _center; }
    Point<T>& getCenter() { return _center; }

    void setDegree(double deg) { _deg = deg; }

private:
    Box<T> _box;
    double _deg;
    Point<T> _center;
};

}  // namespace util

#endif  // UTIL_GEO_BOX_H_
