// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/restrictor.h"
#include <logging/logger.h>

namespace pfaedle::trgraph
{

void restrictor::relax(osmid wid, const trgraph::node* n,
                       const trgraph::edge* e)
{
    // the wid is not unique here, because the OSM ways are split into
    // multiple edges. They are only unique as a pair with a "from-via" point.
    _rlx[node_id_pair(n, wid)] = e;
    auto i = _posDangling.find(node_id_pair(n, wid));
    if (i != _posDangling.end())
    {
        for (const auto& path : i->second)
        {
            _pos[path.first][path.second].second = e;
            assert(path.first->hasEdge(e));
        }
    }

    auto j = _negDangling.find(node_id_pair(n, wid));
    if (j != _negDangling.end())
    {
        for (const auto& path : j->second)
        {
            _neg[path.first][path.second].second = e;
            assert(path.first->hasEdge(e));
        }
    }
}

void restrictor::add(const trgraph::edge* from, osmid to,
                     const trgraph::node* via, bool pos)
{
    const trgraph::edge* toE = nullptr;
    if (_rlx.count(node_id_pair(via, to)))
        toE = _rlx.find(node_id_pair(via, to))->second;
    if (pos)
    {
        _pos[via].push_back(RulePair(from, toE));
        if (!toE)
            _posDangling[node_id_pair(via, to)].push_back(
                    dangling_path(via, _pos[via].size() - 1));
    }
    else
    {
        _neg[via].push_back(RulePair(from, toE));
        if (!toE)
            _negDangling[node_id_pair(via, to)].push_back(
                    dangling_path(via, _neg[via].size() - 1));
    }
}

bool restrictor::may(const trgraph::edge* from,
                     const trgraph::edge* to,
                     const trgraph::node* via) const
{
    auto posI = _pos.find(via);
    auto negI = _neg.find(via);

    if (posI != _pos.end())
    {
        for (const auto& r : posI->second)
        {
            if (r.first == from && r.second && r.second != to)
                return false;
            else if (r.first == from && r.second == to)
                return true;
        }
    }
    if (negI != _neg.end())
    {
        for (const auto& r : negI->second)
        {
            if (r.first == from && r.second == to) return false;
        }
    }
    return true;
}

void restrictor::replace_edge(const trgraph::edge* old,
                             const trgraph::edge* newA,
                             const trgraph::edge* newB)
{
    const trgraph::edge* newFrom;
    const trgraph::edge* newTo;
    if (old->getFrom() == newA->getFrom() || old->getFrom() == newA->getTo())
    {
        newFrom = newA;
        newTo = newB;
    }
    else
    {
        newFrom = newB;
        newTo = newA;
    }
    replace_edge(old, old->getFrom(), newFrom);
    replace_edge(old, old->getTo(), newTo);
}

void restrictor::duplicate_edge(const trgraph::edge* old,
                               const trgraph::edge* newE)
{
    duplicate_edge(old, old->getFrom(), newE);
    duplicate_edge(old, old->getTo(), newE);
}

void restrictor::duplicate_edge(const trgraph::edge* old,
                               const trgraph::node* via,
                               const trgraph::edge* newE)
{
    auto posI = _pos.find(via);
    auto negI = _neg.find(via);

    assert(old->getFrom() == newE->getTo() && old->getTo() == newE->getFrom());

    if (posI != _pos.end())
    {
        for (auto& r : posI->second)
        {
            if (r.first == old)
            {
                if (r.first->getTo() != via)
                {
                    r.first = newE;
                }
            }
            if (r.second == old)
            {
                if (r.second->getFrom() != via)
                {
                    r.second = newE;
                }
            }
        }
    }
    if (negI != _neg.end())
    {
        for (auto& r : negI->second)
        {
            if (r.first == old)
            {
                if (r.first->getTo() != via)
                {
                    r.first = newE;
                }
            }
            if (r.second == old)
            {
                if (r.second->getFrom() != via)
                {
                    r.second = newE;
                }
            }
        }
    }
}

void restrictor::replace_edge(const trgraph::edge* old,
                             const trgraph::node* via,
                             const trgraph::edge* newE)
{
    const auto& pos_iterator = _pos.find(via);
    const auto& neg_iterator = _neg.find(via);

    if (pos_iterator != _pos.end())
    {
        for (auto& r : pos_iterator->second)
        {
            if (r.first == old) r.first = newE;
            if (r.second == old) r.second = newE;
        }
    }
    if (neg_iterator != _neg.end())
    {
        for (auto& r : neg_iterator->second)
        {
            if (r.first == old) r.first = newE;
            if (r.second == old) r.second = newE;
        }
    }
}
}
