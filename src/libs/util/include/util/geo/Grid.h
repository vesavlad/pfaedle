// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GEO_GRID_H_
#define UTIL_GEO_GRID_H_

#include <set>
#include <vector>
#include <map>
#include "util/geo/Geo.h"

namespace util::geo
{

class GridException : public std::runtime_error
{
public:
    explicit GridException(std::string const& msg) :
        std::runtime_error(msg) {}
};

template<typename V, template<typename> class G, typename T>
class Grid
{
public:
    // initialization of a point grid with cell width w and cell height h
    // that covers the area of bounding box bbox
    Grid(double w, double h, const Box<T>& bbox) :
        Grid(w, h, bbox, true)
    {}

    // initialization of a point grid with cell width w and cell height h
    // that covers the area of bounding box bbox
    // optional parameters specifies whether a value->cell index
    // should be kept (true by default!)
    Grid(double w, double h, const Box<T>& bbox, bool buildValIdx) :
        _cellWidth(fabs(w)),
        _cellHeight(fabs(h)),
        _bb(bbox),
        _hasValIdx(buildValIdx)
    {
        _width = bbox.getUpperRight().getX() - bbox.getLowerLeft().getX();
        _height = bbox.getUpperRight().getY() - bbox.getLowerLeft().getY();

        if (_width < 0 || _height < 0)
        {
            _width = 0;
            _height = 0;
            _xWidth = 0;
            _yHeight = 0;
            return;
        }

        _xWidth = ceil(_width / _cellWidth);
        _yHeight = ceil(_height / _cellHeight);

        // resize rows
        _grid.resize(_xWidth);

        // resize columns
        for (size_t i = 0; i < _xWidth; i++)
        {
            _grid[i].resize(_yHeight);
        }
    }

    // the empty grid
    Grid() :
        Grid(true)
    {
    }

    // the empty grid
    Grid(bool buildValIdx) :
        _width(0),
        _height(0),
        _cellWidth(0),
        _cellHeight(0),
        _xWidth(0),
        _yHeight(0),
        _hasValIdx(buildValIdx)
    {
    }

    // add object t to this grid
    void add(G<T> geom, V val)
    {
        Box<T> box = getBoundingBox(geom);
        size_t swX = getCellXFromX(box.getLowerLeft().getX());
        size_t swY = getCellYFromY(box.getLowerLeft().getY());

        size_t neX = getCellXFromX(box.getUpperRight().getX());
        size_t neY = getCellYFromY(box.getUpperRight().getY());

        for (size_t x = swX; x <= neX && x < _grid.size(); x++)
        {
            for (size_t y = swY; y <= neY && y < _grid[x].size(); y++)
            {
                if (intersects(geom, getBox(x, y)))
                {
                    add(x, y, val);
                }
            }
        }
    }
    void add(size_t x, size_t y, V val)
    {
        _grid[x][y].insert(val);
        if (_hasValIdx) _index[val].insert(std::pair<size_t, size_t>(x, y));
    }

    void get(const Box<T>& box, std::set<V>* s) const
    {
        size_t swX = getCellXFromX(box.getLowerLeft().getX());
        size_t swY = getCellYFromY(box.getLowerLeft().getY());

        size_t neX = getCellXFromX(box.getUpperRight().getX());
        size_t neY = getCellYFromY(box.getUpperRight().getY());

        for (size_t x = swX; x <= neX && x >= 0 && x < _xWidth; x++)
        {
            for (size_t y = swY; y <= neY && y >= 0 && y < _yHeight; y++)
            {
                get(x, y, s);
            }
        }
    }

    void get(const G<T>& geom, double d, std::set<V>* s) const
    {
        Box<T> a = getBoundingBox(geom);
        Box<T> b(Point<T>(a.getLowerLeft().getX() - d,
                          a.getLowerLeft().getY() - d),
                 Point<T>(a.getUpperRight().getX() + d,
                          a.getUpperRight().getY() + d));
        return get(b, s);
    }
    void get(size_t x, size_t y, std::set<V>* s) const
    {
        if (_hasValIdx)
        {
            s->insert(_grid[x][y].begin(), _grid[x][y].end());
        }
        else
        {
            // if we dont have a value index, we have a set of deleted nodes.
            // in this case, only insert if not deleted
            std::copy_if(_grid[x][y].begin(), _grid[x][y].end(),
                         std::inserter(*s, s->end()),
                         [&](const V& v) { return Grid<V, G, T>::_removed.count(v) == 0; });
        }
    }
    void remove(V val)
    {
        if (_hasValIdx)
        {
            auto i = _index.find(val);
            if (i == _index.end()) return;

            for (auto pair : i->second)
            {
                _grid[pair.first][pair.second].erase(
                        _grid[pair.first][pair.second].find(val));
            }

            _index.erase(i);
        }
        else
        {
            _removed.insert(val);
        }
    }

    void getNeighbors(const V& val, double d, std::set<V>* s) const
    {
        if (!_hasValIdx)
            throw GridException("No value index build!");

        auto it = _index.find(val);
        if (it == _index.end())
            return;

        size_t xPerm = ceil(d / _cellWidth);
        size_t yPerm = ceil(d / _cellHeight);

        for (auto pair : it->second)
        {
            getCellNeighbors(pair.first, pair.second, xPerm, yPerm, s);
        }
    }
    void getCellNeighbors(const V& val, size_t d, std::set<V>* s) const
    {
        if (!_hasValIdx)
            throw GridException("No value index build!");

        auto it = _index.find(val);
        if (it == _index.end())
            return;

        for (auto pair : it->second)
        {
            getCellNeighbors(pair.first, pair.second, d, d, s);
        }
    }
    void getCellNeighbors(size_t cx, size_t cy, size_t xPerm, size_t yPerm, std::set<V>* s) const
    {
        size_t swX = xPerm > cx ? 0 : cx - xPerm;
        size_t swY = yPerm > cy ? 0 : cy - yPerm;

        size_t neX = xPerm + cx + 1 > _xWidth ? _xWidth : cx + xPerm + 1;
        size_t neY = yPerm + cy + 1 > _yHeight ? _yHeight : cy + yPerm + 1;

        for (size_t x = swX; x < neX; x++)
        {
            for (size_t y = swY; y < neY; y++)
            {
                s->insert(_grid[x][y].begin(), _grid[x][y].end());
            }
        }
    }

    std::set<std::pair<size_t, size_t>> getCells(const V& val) const
    {
        if (!_hasValIdx)
            throw GridException("No value index build!");
        return _index.find(val)->second;
    }

    size_t getXWidth() const
    {

        return _xWidth;
    }
    size_t getYHeight() const
    {
        return _yHeight;
    }

private:
    double _width;
    double _height;

    double _cellWidth;
    double _cellHeight;

    Box<T> _bb;

    size_t _counter;

    size_t _xWidth;
    size_t _yHeight;

    bool _hasValIdx;

    std::vector<std::vector<std::set<V>>> _grid;
    std::map<V, std::set<std::pair<size_t, size_t>>> _index;
    std::set<V> _removed;

    Box<T> getBox(size_t x, size_t y) const
    {
        Point<T> sw(_bb.getLowerLeft().getX() + x * _cellWidth,
                    _bb.getLowerLeft().getY() + y * _cellHeight);
        Point<T> ne(_bb.getLowerLeft().getX() + (x + 1) * _cellWidth,
                    _bb.getLowerLeft().getY() + (y + 1) * _cellHeight);
        return Box<T>(sw, ne);
    }

    size_t getCellXFromX(double lon) const
    {
        float dist = lon - _bb.getLowerLeft().getX();
        if (dist < 0) dist = 0;
        return floor(dist / _cellWidth);
    }
    size_t getCellYFromY(double lat) const
    {
        float dist = lat - _bb.getLowerLeft().getY();
        if (dist < 0) dist = 0;
        return floor(dist / _cellHeight);
    }
};

}  // namespace util

#endif  // UTIL_GEO_GRID_H_
