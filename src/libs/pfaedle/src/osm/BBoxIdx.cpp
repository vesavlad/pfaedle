// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/osm/BBoxIdx.h"

namespace pfaedle::osm
{


BBoxIdx::BBoxIdx(double padding) :
    _padding(padding),
    _size(0)
{}


void BBoxIdx::add(util::geo::Box<double> box)
{
    // division by 83.000m is only correct here around a latitude deg of 25,
    // but should be a good heuristic. 1 deg is around 63km at latitude deg of 44,
    // and 110 at deg=0, since we usually dont do map matching in the arctic,
    // its okay to use 83km here.
    box = util::geo::pad(box, _padding / 83000);
    addToTree(box, &_root, 0);
    _size++;
}


size_t BBoxIdx::size() const { return _size; }


bool BBoxIdx::contains(const util::geo::Point<double>& p) const
{
    return treeHas(p, _root);
}


BOX BBoxIdx::getFullWebMercBox() const
{
    return BOX(
            util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(
                    _root.box.getLowerLeft().getY(), _root.box.getLowerLeft().getX()),
            util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(
                    _root.box.getUpperRight().getY(), _root.box.getUpperRight().getX()));
}


BOX BBoxIdx::getFullBox() const { return _root.box; }


std::vector<util::geo::Box<double>> BBoxIdx::getLeafs() const
{
    std::vector<util::geo::Box<double>> ret;
    getLeafsRec(_root, &ret);
    return ret;
}


void BBoxIdx::getLeafsRec(const BBoxIdxNd& nd,
                          std::vector<util::geo::Box<double>>* ret) const
{
    if (nd.childs.empty())
    {
        ret->push_back(nd.box);
        return;
    }

    for (const auto& child : nd.childs)
    {
        getLeafsRec(child, ret);
    }
}


bool BBoxIdx::treeHas(const util::geo::Point<double>& p, const BBoxIdxNd& nd) const
{
    if (nd.childs.empty()) return util::geo::contains(p, nd.box);
    for (const auto& child : nd.childs)
    {
        if (util::geo::contains(p, child.box)) return treeHas(p, child);
    }

    return false;
}


void BBoxIdx::addToTree(const util::geo::Box<double>& box, BBoxIdxNd* nd, size_t lvl)
{
    double best_common_area = 0;
    ssize_t best_child = -1;

    // 1. update the bbox of this node
    nd->box = util::geo::extendBox(box, nd->box);

    if (lvl == MAX_LVL) return;

    // 2. find best candidate
    for (size_t i = 0; i < nd->childs.size(); i++)
    {
        double cur = util::geo::commonArea(box, nd->childs[i].box);
        if (cur > MIN_COM_AREA && cur > best_common_area)
        {
            best_child = i;
            best_common_area = cur;
        }
    }

    if (best_child < 0)
    {
        // 3. add a new node with the inserted bbox
        nd->childs.emplace_back(box);
        addToTree(box, &nd->childs.back(), lvl + 1);
    }
    else
    {
        // 3. add to best node
        addToTree(box, &nd->childs[best_child], lvl + 1);
    }

    // TODO(patrick): some tree balancing by mergin overlapping bboxes in
    // non-leafs
}
}
