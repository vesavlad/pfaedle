// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GEO_POLYLINE_H_
#define UTIL_GEO_POLYLINE_H_

#include <util/geo/Geo.h>
#include <cfloat>
#include <ostream>
#include <iomanip>
#include <string>
#include <set>
#include <vector>

namespace util::geo
{

static const double MAX_EQ_DISTANCE = 15;
static const double AVERAGING_STEP = 20;

// legacy code, will be removed in the future

template<typename T>
struct LinePoint
{
    LinePoint() :
        lastIndex(0),
        totalPos(-1),
        p()
    {}

    LinePoint(size_t i, double pos, const Point<T>& p) :
        lastIndex(i),
        totalPos(pos), 
        p(p)
    {}

    size_t lastIndex;
    double totalPos;
    Point<T> p;
};

template<typename T>
struct LinePointCmp
{
    bool operator()(const LinePoint<T>& lh, const LinePoint<T>& rh) const
    {
        return lh.totalPos < rh.totalPos;
    }
};


template<typename T>
using LinePointPair = std::pair<LinePoint<T>, LinePoint<T>>;
template<typename T>
using SharedSegment = std::pair<LinePointPair<T>, LinePointPair<T>>;

template<typename T>
struct SharedSegments
{
    std::vector<SharedSegment<T>> segments;
};

template<typename T>
class PolyLine
{
public:
    PolyLine();
    PolyLine(const Point<T>& from, const Point<T>& to);
    PolyLine(const Line<T>& l);

    PolyLine& operator<<(const Point<T>& p);
    PolyLine& operator>>(const Point<T>& p);

    void reverse();
    PolyLine getReversed() const;

    void offsetPerp(double units);

    PolyLine getPerpOffsetted(double units) const;

    const Line<T>& getLine() const;

    double distTo(const PolyLine<T>& g) const;
    double distTo(const Point<T>& p) const;

    SharedSegments<T> getSharedSegments(const PolyLine<T>& pl, double dmax) const;

    double getLength() const;

    // return point at dist
    LinePoint<T> getPointAtDist(double dist) const;

    // return point at [0..1]
    LinePoint<T> getPointAt(double dist) const;

    PolyLine<T> getSegment(double a, double b) const;
    PolyLine<T> getSegmentAtDist(double dista, double distb) const;
    PolyLine<T> getSegment(const LinePoint<T>& start, const LinePoint<T>& end) const;
    PolyLine<T> getSegment(const Point<T>& a, const Point<T>& b) const;

    std::set<LinePoint<T>, LinePointCmp<T>> getIntersections(const PolyLine<T>& g) const;

    static PolyLine<T> average(const std::vector<const PolyLine<T>*>& lines);
    static PolyLine<T> average(const std::vector<const PolyLine<T>*>& lines,
                               const std::vector<double>& weights);

    void simplify(double d);
    void empty();

    void smoothenOutliers(double d);

    std::pair<size_t, double> nearestSegment(const Point<T>& p) const;
    std::pair<size_t, double> nearestSegmentAfter(const Point<T>& p,
                                                  size_t after) const;

    LinePoint<T> projectOn(const Point<T>& p) const;
    LinePoint<T> projectOnAfter(const Point<T>& p, size_t after) const;

    void move(double vx, double vy);

    std::pair<double, double> getSlopeBetween(double ad, double bd) const;
    std::pair<double, double> getSlopeBetweenDists(double ad, double bd) const;

    // equality operator, will hold frechet-distance equality check in
    // the dmax
    bool operator==(const PolyLine& rhs) const;
    bool contains(const PolyLine& rhs, double dmax) const;
    bool equals(const PolyLine& rhs) const;
    bool equals(const PolyLine& rhs, double dmax) const;

    std::string getWKT() const;

    PolyLine getOrthoLineAtDist(double d, double lengt) const;

    Point<T> interpolate(const Point<T>& a, const Point<T>& b, double p) const;

    void fixTopology(double maxl);
    void applyChaikinSmooth(size_t depth);

    const Point<T>& front() const;
    const Point<T>& back() const;

private:
    std::set<LinePoint<T>, LinePointCmp<T>> getIntersections(const PolyLine& p,
                                                             size_t a, size_t b) const;
    Line<T> _line;
};

template<typename T>
PolyLine<T>::PolyLine()
{}

template<typename T>
PolyLine<T>::PolyLine(const Point<T>& from, const Point<T>& to)
{
    *this << from << to;
}

template<typename T>
PolyLine<T>::PolyLine(const Line<T>& l) :
    _line(l)
{}

template<typename T>
PolyLine<T>& PolyLine<T>::operator<<(const Point<T>& p)
{
    _line.push_back(p);
    return *this;
}

template<typename T>
PolyLine<T>& PolyLine<T>::operator>>(const Point<T>& p)
{
    _line.insert(_line.begin(), p);
    return *this;
}

template<typename T>
void PolyLine<T>::reverse()
{
    std::reverse(_line.begin(), _line.end());
}

template<typename T>
PolyLine<T> PolyLine<T>::getReversed() const
{
    PolyLine ret = *this;
    ret.reverse();
    return ret;
}

template<typename T>
const Line<T>& PolyLine<T>::getLine() const
{
    return _line;
}

template<typename T>
PolyLine<T> PolyLine<T>::getPerpOffsetted(double units) const
{
    PolyLine p = *this;
    p.offsetPerp(units);
    return p;
}

template<typename T>
void PolyLine<T>::offsetPerp(double units)
{
    /*
     * calculate perpendicular offset of a polyline
     *
     * there doesn't seem to be any library which reliably does that,
     * so we do it ourself here until we find one...
     */

    if (fabs(units) < 0.001) return;

    assert(getLength() > 0);

    if (_line.size() < 2) return;

    Line<T> ret;
    Point<T> lastP = _line.front();

    Point<T>*lastIns = 0, *befLastIns = 0;

    for (size_t i = 1; i < _line.size(); i++)
    {
        Point<T> curP = _line[i];

        double n1 = lastP.getY() - curP.getY();
        double n2 = curP.getX() - lastP.getX();
        double n = sqrt(n1 * n1 + n2 * n2);

        // if n == 0, the segment is effectively a point
        // we would get into all sorts of troubles if we tried to offset a point...
        if (!(n > 0)) continue;

        n1 = n1 / n;
        n2 = n2 / n;

        lastP.setX(lastP.getX() + (n1 * units));
        lastP.setY(lastP.getY() + (n2 * units));

        curP.setX(curP.getX() + (n1 * units));
        curP.setY(curP.getY() + (n2 * units));

        if (lastIns && befLastIns &&
            lineIntersects(*lastIns, *befLastIns, lastP, curP))
        {
            *lastIns = intersection(*lastIns, *befLastIns, lastP, curP);

            double d = dist(lastP, *lastIns);
            double d2 = distToSegment(*lastIns, *befLastIns, lastP);

            if (d > fabs(units) * 2 && d2 < d - (fabs(units)))
            {
                PolyLine pl(*lastIns, *befLastIns);
                PolyLine pll(*lastIns, curP);
                pl = pl.getSegment(0, (d - (fabs(units))) / pl.getLength());
                pll = pll.getSegment(0, (d - (fabs(units))) / pll.getLength());

                ret.push_back(pll.back());
                *lastIns = pl.back();

                ret.push_back(curP);
            }
            else
            {
                ret.push_back(curP);
            }
        }
        else
        {
            ret.push_back(lastP);
            ret.push_back(curP);
        }

        lastIns = &ret[ret.size() - 1];
        befLastIns = &ret[ret.size() - 2];

        lastP = _line[i];
    }

    _line = ret;

    // heuristics
    simplify(1);
    fixTopology(fabs(2 * 3.14 * units));
}

template<typename T>
PolyLine<T> PolyLine<T>::getSegment(double a, double b) const
{
    if (a > b)
    {
        double c = a;
        a = b;
        b = c;
    }
    LinePoint<T> start = getPointAt(a);
    LinePoint<T> end = getPointAt(b);

    return getSegment(start, end);
}

template<typename T>
PolyLine<T> PolyLine<T>::getSegmentAtDist(double a, double b) const
{
    if (a > b)
    {
        double c = a;
        a = b;
        b = c;
    }
    LinePoint<T> start = getPointAtDist(a);
    LinePoint<T> end = getPointAtDist(b);

    return getSegment(start, end);
}

template<typename T>
PolyLine<T> PolyLine<T>::getSegment(const Point<T>& a,
                                    const Point<T>& b) const
{
    LinePoint<T> start = projectOn(a);
    LinePoint<T> end = projectOnAfter(b, start.lastIndex);

    return getSegment(start, end);
}

template<typename T>
PolyLine<T> PolyLine<T>::getSegment(const LinePoint<T>& start,
                                    const LinePoint<T>& end) const
{
    PolyLine ret;
    ret << start.p;

    if (start.lastIndex + 1 <= end.lastIndex)
    {
        ret._line.insert(ret._line.end(), _line.begin() + start.lastIndex + 1,
                         _line.begin() + end.lastIndex + 1);
    }
    ret << end.p;

    // find a more performant way to clear the result of above
    ret.simplify(0);

    return ret;
}

template<typename T>
LinePoint<T> PolyLine<T>::getPointAtDist(double atDist) const
{
    if (atDist > getLength()) atDist = getLength();
    if (atDist < 0) atDist = 0;

    double dist = 0;

    if (_line.size() == 1) return LinePoint<T>(0, 0, _line[0]);

    const Point<T>* last = &_line[0];

    for (size_t i = 1; i < _line.size(); i++)
    {
        const Point<T>& cur = _line[i];
        double d = geo::dist(*last, cur);
        dist += d;

        if (dist > atDist)
        {
            double p = (d - (dist - atDist));
            return LinePoint<T>(i - 1, atDist / getLength(),
                                interpolate(*last, cur, p));
        }

        last = &_line[i];
    }

    return LinePoint<T>(_line.size() - 1, 1, _line.back());
}

template<typename T>
LinePoint<T> PolyLine<T>::getPointAt(double at) const
{
    at *= getLength();
    return getPointAtDist(at);
}

template<typename T>
Point<T> PolyLine<T>::interpolate(const Point<T>& a, const Point<T>& b,
                                  double p) const
{
    double n1 = b.getX() - a.getX();
    double n2 = b.getY() - a.getY();
    double n = sqrt(n1 * n1 + n2 * n2);
    n1 = n1 / n;
    n2 = n2 / n;
    return Point<T>(a.getX() + (n1 * p),
                    a.getY() + (n2 * p));
}

template<typename T>
double PolyLine<T>::distTo(const PolyLine<T>& g) const
{
    return dist(_line, g.getLine());
}

template<typename T>
double PolyLine<T>::distTo(const Point<T>& p) const
{
    return dist(_line, p);
}

template<typename T>
double PolyLine<T>::getLength() const
{
    return len(_line);
}

template<typename T>
PolyLine<T> PolyLine<T>::average(const std::vector<const PolyLine<T>*>& lines,
                                 const std::vector<double>& weights)
{
    bool weighted = lines.size() == weights.size();
    double stepSize;

    double longestLength = DBL_MIN;// avoid recalc of length on each comparision
    for (const PolyLine* p : lines)
    {
        if (p->getLength() > longestLength)
        {
            longestLength = p->getLength();
        }
    }

    PolyLine ret;
    double total = 0;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (weighted)
        {
            total += weights[i];
        }
        else
        {
            total += 1;
        }
    }

    stepSize = AVERAGING_STEP / longestLength;
    bool end = false;
    for (double a = 0; !end; a += stepSize)
    {
        if (a > 1)
        {
            a = 1;
            end = true;
        }
        double x = 0, y = 0;

        for (size_t i = 0; i < lines.size(); ++i)
        {
            const PolyLine* pl = lines[i];
            Point<T> p = pl->getPointAt(a).p;
            if (weighted)
            {
                x += p.getX() * weights[i];
                y += p.getY() * weights[i];
            }
            else
            {
                x += p.getX();
                y += p.getY();
            }
        }
        ret << Point<T>(x / total, y / total);
    }

    ret.simplify(0);

    return ret;
}

template<typename T>
PolyLine<T> PolyLine<T>::average(const std::vector<const PolyLine<T>*>& lines)
{
    return average(lines, std::vector<double>());
}

template<typename T>
std::pair<size_t, double> PolyLine<T>::nearestSegmentAfter(const Point<T>& p,
                                                           size_t a) const
{
    // returns the index of the starting point of the nearest segment of p
    assert(a < _line.size());

    double totalLength = getLength();
    size_t smallest = a;
    double totalDist = 0;
    double dist = DBL_MAX;
    double smallestDist = 0;

    for (size_t i = smallest + 1; i < _line.size(); i++)
    {
        Point<T> startP(_line[i - 1]);
        Point<T> endP(_line[i]);

        if (i > 1)
        {
            totalDist += geo::dist(_line[i - 2], _line[i - 1]);
        }

        double curDist = distToSegment(startP, endP, p);

        if (curDist < dist)
        {
            dist = curDist;
            smallest = i - 1;
            smallestDist = totalDist;
        }
    }

    if (totalLength > 0)
    {
        smallestDist /= totalLength;
    }
    else
    {
        smallestDist = 0;
    }

    return std::pair<size_t, double>(smallest, smallestDist);
}

template<typename T>
std::pair<size_t, double> PolyLine<T>::nearestSegment(const Point<T>& p) const
{
    return nearestSegmentAfter(p, 0);
}

template<typename T>
LinePoint<T> PolyLine<T>::projectOn(const Point<T>& p) const
{
    return projectOnAfter(p, 0);
}

template<typename T>
LinePoint<T> PolyLine<T>::projectOnAfter(const Point<T>& p, size_t a) const
{
    assert(a < _line.size());
    std::pair<size_t, double> bc = nearestSegmentAfter(p, a);

    Point<T> ret = geo::projectOn(_line[bc.first], p, _line[bc.first + 1]);

    if (getLength() > 0)
    {
        bc.second += dist(_line[bc.first], ret) / getLength();
    }

    return LinePoint<T>(bc.first, bc.second, ret);
}

template<typename T>
void PolyLine<T>::simplify(double d)
{
    _line = geo::simplify(_line, d);
}

template<typename T>
void PolyLine<T>::smoothenOutliers(double d)
{
    if (_line.size() < 3) return;
    for (size_t i = 1; i < _line.size() - 3; ++i)
    {
        double ang = innerProd(_line[i], _line[i - 1], _line[i + 1]);

        if (dist(_line[i], _line[i + 1]) < d || dist(_line[i], _line[i - 1]) < d)
        {
            if (ang < 35)
            {
                _line.erase(_line.begin() + i);
            }
        }
    }
}

template<typename T>
bool PolyLine<T>::equals(const PolyLine<T>& rhs) const
{
    // TODO: why 100? make global static or configurable or determine in some
    //       way!
    return equals(rhs, 100);
}

template<typename T>
bool PolyLine<T>::operator==(const PolyLine<T>& rhs) const
{
    // TODO: why 100? make global static or configurable or determine in some
    //       way!
    return equals(rhs, 100);
}

template<typename T>
bool PolyLine<T>::equals(const PolyLine<T>& rhs, double dmax) const
{
    // check if two lines are equal, THE DIRECTION DOES NOT MATTER HERE!!!!!

    if (_line.size() == 2 && _line.size() == rhs.getLine().size())
    {
        // trivial case, straight line, implement directly
        return (dist(_line[0], rhs.getLine()[0]) < dmax &&
                dist(_line.back(), rhs.back()) < dmax) ||
               (dist(_line[0], rhs.back()) < dmax &&
                dist(_line.back(), rhs.getLine()[0]) < dmax);
    }
    else
    {
        return contains(rhs, dmax) && rhs.contains(*this, dmax);
    }

    return true;
}

template<typename T>
bool PolyLine<T>::contains(const PolyLine<T>& rhs, double dmax) const
{
    // check if two lines are equal. Line direction does not matter here.

    for (size_t i = 0; i < rhs.getLine().size(); ++i)
    {
        double d = dist(rhs.getLine()[i], getLine());
        if (d > dmax)
        {
            return false;
        }
    }

    return true;
}

template<typename T>
void PolyLine<T>::move(double vx, double vy)
{
    for (size_t i = 0; i < _line.size(); i++)
    {
        _line[i].setX(_line[i].getX() + vx);
        _line[i].setY(_line[i].getY() + vy);
    }
}

template<typename T>
SharedSegments<T> PolyLine<T>::getSharedSegments(const PolyLine<T>& pl,
                                                 double dmax) const
{
    /**
     * Returns the segments this polyline share with pl
     * atm, this is a very simple distance-based algorithm
     */
    double STEP_SIZE = 2;
    double MAX_SKIPS = 4;
    double MIN_SEG_LENGTH = 1;// dmax / 2;  // make this configurable!

    SharedSegments<T> ret;

    if (distTo(pl) > dmax) return ret;

    bool in = false, single = true;
    double curDist = 0;
    double curTotalSegDist = 0;
    size_t skips;

    LinePoint<T> curStartCand, curEndCand, curStartCandCmp, curEndCandCmp;

    double comp = 0, curSegDist = 0;
    double length = getLength(), plLength = pl.getLength();

    for (size_t i = 1; i < _line.size(); ++i)
    {
        const Point<T>& s = _line[i - 1];
        const Point<T>& e = _line[i];

        bool lastRound = false;

        double totalDist = dist(s, e);
        while (curSegDist <= totalDist)
        {
            const auto& curPointer = interpolate(s, e, curSegDist);

            if (pl.distTo(curPointer) <= dmax)
            {
                LinePoint<T> curCmpPointer = pl.projectOn(curPointer);
                LinePoint<T> curBackProjectedPointer = projectOn(curCmpPointer.p);


                skips = 0;

                if (in)
                {
                    curEndCand = curBackProjectedPointer;
                    curEndCandCmp = curCmpPointer;

                    if (curEndCand.totalPos < curStartCand.totalPos)
                    {
                        curEndCand = curStartCand;
                    }

                    single = false;

                    comp = fabs(curStartCand.totalPos * length -
                                curEndCand.totalPos * length) /
                           fabs(curStartCandCmp.totalPos * plLength -
                                curEndCandCmp.totalPos * plLength);
                }
                else
                {
                    in = true;
                    curStartCand = curBackProjectedPointer;
                    curStartCandCmp = curCmpPointer;
                }
            }
            else
            {
                if (in)
                {
                    skips++;
                    if (skips > MAX_SKIPS)
                    {// TODO: make configurable
                        if (comp > 0.8 && comp < 1.2 && !single &&
                            (fabs(curStartCand.totalPos * length -
                                  curEndCand.totalPos * length) > MIN_SEG_LENGTH &&
                             fabs(curStartCandCmp.totalPos * plLength -
                                  curEndCandCmp.totalPos * plLength) > MIN_SEG_LENGTH))
                        {
                            assert(curStartCand.totalPos < curEndCand.totalPos);
                            ret.segments.push_back(
                                    SharedSegment<T>(std::pair<LinePoint<T>, LinePoint<T>>(
                                                             curStartCand, curStartCandCmp),
                                                     std::pair<LinePoint<T>, LinePoint<T>>(
                                                             curEndCand, curEndCandCmp)));

                            // TODO: only return the FIRST one, make this configuralbe
                            return ret;
                        }

                        in = false;
                        single = true;
                    }
                }
            }

            if (curSegDist + STEP_SIZE > totalDist && !lastRound)
            {
                lastRound = true;
                double finalStep = totalDist - curSegDist - 0.0005;
                curSegDist += finalStep;
                curDist += finalStep;
            }
            else
            {
                curSegDist += STEP_SIZE;
                curDist += STEP_SIZE;
            }
        }

        curSegDist = curSegDist - totalDist;
        curTotalSegDist += totalDist;
    }

    if (comp > 0.8 && comp < 1.2 && in && !single &&
        (fabs(curStartCand.totalPos * length - curEndCand.totalPos * length) >
                 MIN_SEG_LENGTH &&
         fabs(curStartCandCmp.totalPos * plLength -
              curEndCandCmp.totalPos * plLength) > MIN_SEG_LENGTH))
    {
        assert(curStartCand.totalPos < curEndCand.totalPos);
        ret.segments.push_back(SharedSegment<T>(
                std::pair<LinePoint<T>, LinePoint<T>>(curStartCand, curStartCandCmp),
                std::pair<LinePoint<T>, LinePoint<T>>(curEndCand, curEndCandCmp)));
    }

    return ret;
}

template<typename T>
std::set<LinePoint<T>, LinePointCmp<T>> PolyLine<T>::getIntersections(
        const PolyLine<T>& g) const
{
    std::set<LinePoint<T>, LinePointCmp<T>> ret;

    for (size_t i = 1; i < g.getLine().size(); ++i)
    {
        // for each line segment, check if it intersects with a line segment in g
        const std::set<LinePoint<T>, LinePointCmp<T>> a =
                getIntersections(g, i - 1, i);
        ret.insert(a.begin(), a.end());
    }

    return ret;
}

template<typename T>
std::set<LinePoint<T>, LinePointCmp<T>> PolyLine<T>::getIntersections(
        const PolyLine<T>& p, size_t a, size_t b) const
{
    std::set<LinePoint<T>, LinePointCmp<T>> ret;

    if (dist(p.getLine()[a], p.getLine()[b]) == 0)
    {
        // we cannot intersect with a point
        return ret;
    }

    for (size_t i = 1; i < _line.size(); ++i)
    {
        if (intersects(_line[i - 1], _line[i], p.getLine()[a], p.getLine()[b]))
        {
            Point<T> isect =
                    intersection(_line[i - 1], _line[i], p.getLine()[a], p.getLine()[b]);
            ret.insert(p.projectOn(isect));
        }
    }

    return ret;
}

template<typename T>
PolyLine<T> PolyLine<T>::getOrthoLineAtDist(double d, double length) const
{
    Point<T> avgP = getPointAtDist(d).p;

    double angle = angBetween(getPointAtDist(d - 5).p, getPointAtDist(d + 5).p);

    double angleX1 = avgP.getX() + cos(angle + M_PI / 2) * length / 2;
    double angleY1 = avgP.getY() + sin(angle + M_PI / 2) * length / 2;

    double angleX2 = avgP.getX() + cos(angle + M_PI / 2) * -length / 2;
    double angleY2 = avgP.getY() + sin(angle + M_PI / 2) * -length / 2;

    return PolyLine(Point<T>(angleX1, angleY1), Point<T>(angleX2, angleY2));
}

template<typename T>
void PolyLine<T>::empty()
{
    _line.empty();
}

template<typename T>
std::pair<double, double> PolyLine<T>::getSlopeBetween(double ad,
                                                       double bd) const
{
    LinePoint<T> a = getPointAt(ad);
    LinePoint<T> b = getPointAt(bd);

    double d = dist(a.p, b.p);

    double dx = (b.p.getX() - a.p.getX()) / d;
    double dy = (b.p.getY() - a.p.getY()) / d;

    return std::pair<double, double>(dx, dy);
}

template<typename T>
std::pair<double, double> PolyLine<T>::getSlopeBetweenDists(double ad,
                                                            double bd) const
{
    return getSlopeBetween(ad / getLength(), bd / getLength());
}

template<typename T>
std::string PolyLine<T>::getWKT() const
{
    std::stringstream ss;
    ss << std::setprecision(12) << geo::getWKT(_line);

    return ss.str();
}

template<typename T>
void PolyLine<T>::fixTopology(double maxl)
{
    double distA = 0;

    for (size_t i = 1; i < _line.size() - 1; i++)
    {
        double distB =
                distA + dist(_line[i - 1], _line[i]) + dist(_line[i], _line[i + 1]);
        for (size_t j = i + 2; j < _line.size(); j++)
        {
            if (intersects(_line[i - 1], _line[i], _line[j - 1], _line[j]))
            {
                Point<T> p =
                        intersection(_line[i - 1], _line[i], _line[j - 1], _line[j]);

                double posA = dist(_line[i - 1], p) + distA;
                double posB = dist(_line[j - 1], p) + distB;

                if (fabs(posA - posB) < maxl)
                {
                    _line[i] = p;
                    _line.erase(_line.begin() + i + 1, _line.begin() + j);
                }
            }

            distB += dist(_line[j - 1], _line[j]);
        }
        distA += dist(_line[i - 1], _line[i]);
    }
}

template<typename T>
void PolyLine<T>::applyChaikinSmooth(size_t depth)
{
    for (size_t i = 0; i < depth; i++)
    {
        Line<T> smooth;

        smooth.push_back(_line.front());

        for (size_t i = 1; i < _line.size(); i++)
        {
            Point<T> pA = _line[i - 1];
            Point<T> pB = _line[i];

            smooth.push_back(
                    Point<T>(0.75 * pA.getX() + 0.25 * pB.getX(),
                             0.75 * pA.getY() + 0.25 * pB.getY()));
            smooth.push_back(
                    Point<T>(0.25 * pA.getX() + 0.75 * pB.getX(),
                             0.25 * pA.getY() + 0.75 * pB.getY()));
        }

        smooth.push_back(_line.back());
        _line = smooth;
    }
}

template<typename T>
const Point<T>& PolyLine<T>::front() const
{
    return _line.front();
}

template<typename T>
const Point<T>& PolyLine<T>::back() const
{
    return _line.back();
}

}  // namespace util

#endif  // UTIL_GEO_POLYLINE_H_
