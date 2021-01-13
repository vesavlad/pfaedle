// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_BBOXIDX_H_
#define PFAEDLE_OSM_BBOXIDX_H_

#include "pfaedle/definitions.h"
#include "util/geo/Geo.h"
#include <vector>

namespace pfaedle::osm
{

struct BBoxIdxNd
{
    BBoxIdxNd() :
        box(util::geo::minbox<double>())
    {}

    BBoxIdxNd(const BOX& box) :
        box(box)
    {}

    BOX box;
    std::vector<BBoxIdxNd> childs;
};

/*
 * Poor man's R-tree
 */
class BBoxIdx
{
public:
    explicit BBoxIdx(double padding);

    // Add a bounding box to this index
    void add(BOX box);

    // Check if a point is contained in this index
    bool contains(const POINT& box) const;

    // Return the full total bounding box of this index
    BOX get_full_web_merc_box() const;

    // Return the full total bounding box of this index
    BOX get_full_box() const;

    // Return the size of this index
    size_t size() const;

    // return the leaf bounding boxes of this idx
    std::vector<BOX> get_leafs() const;

private:
    void add_to_tree(const BOX& box, BBoxIdxNd* nd, size_t lvl);
    bool tree_has(const POINT& p, const BBoxIdxNd& nd) const;

    void get_leafs_rec(const BBoxIdxNd& nd, std::vector<BOX>* ret) const;

    static const size_t MAX_LVL = 5;
    static constexpr double MIN_COM_AREA = 0.0;

    double _padding;
    size_t _size;

    BBoxIdxNd _root;
};
}  // namespace pfaedle::osm

#endif  // PFAEDLE_OSM_BBOXIDX_H_
