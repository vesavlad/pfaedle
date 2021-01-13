// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include <pfaedle/osm/BBoxIdx.h>

namespace pfaedle::osm
{

BBoxIdx::BBoxIdx(double padding) :
    _padding(padding),
    _size(0)
{}


void BBoxIdx::add(BOX box)
{
    // division by 83.000m is only correct here around a latitude deg of 25,
    // but should be a good heuristic. 1 deg is around 63km at latitude deg of 44,
    // and 110 at deg=0, since we usually dont do map matching in the arctic,
    // its okay to use 83km here.
    box = util::geo::pad(box, _padding / 83000);
    add_to_tree(box, &_root, 0);
    _size++;
}


size_t BBoxIdx::size() const
{
    return _size;
}


bool BBoxIdx::contains(const POINT& p) const
{
    return tree_has(p, _root);
}


BOX BBoxIdx::get_full_web_merc_box() const
{
    return BOX(
            util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(
                    _root.box.getLowerLeft().getY(), _root.box.getLowerLeft().getX()),
            util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(
                    _root.box.getUpperRight().getY(), _root.box.getUpperRight().getX()));
}


BOX BBoxIdx::get_full_box() const
{
    return _root.box;
}


std::vector<BOX> BBoxIdx::get_leafs() const
{
    std::vector<util::geo::Box<double>> ret;
    get_leafs_rec(_root, &ret);
    return ret;
}


void BBoxIdx::get_leafs_rec(const BBoxIdxNd& nd, std::vector<BOX>* ret) const
{
    if (nd.childs.empty())
    {
        ret->push_back(nd.box);
        return;
    }

    for (const auto& child : nd.childs)
    {
        get_leafs_rec(child, ret);
    }
}


bool BBoxIdx::tree_has(const POINT& p, const BBoxIdxNd& nd) const
{
    if (nd.childs.empty())
        return util::geo::contains(p, nd.box);

    for (const auto& child : nd.childs)
    {
        if (util::geo::contains(p, child.box))
            return tree_has(p, child);
    }

    return false;
}


void BBoxIdx::add_to_tree(const BOX& box, BBoxIdxNd* nd, size_t lvl)
{
    double best_common_area = 0;
    ssize_t best_child = -1;

    // 1. update the bbox of this node
    nd->box = util::geo::extendBox(box, nd->box);

    if (lvl == MAX_LVL)
        return;

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
        add_to_tree(box, &nd->childs.back(), lvl + 1);
    }
    else
    {
        // 3. add to best node
        add_to_tree(box, &nd->childs[best_child], lvl + 1);
    }

    // TODO(patrick): some tree balancing by mergin overlapping bboxes in
    // non-leafs
}
}
