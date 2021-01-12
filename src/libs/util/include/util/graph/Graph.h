// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GRAPH_GRAPH_H_
#define UTIL_GRAPH_GRAPH_H_

#include <set>
#include <string>
#include <iostream>
#include <cassert>

#include "util/graph/Edge.h"
#include "util/graph/Node.h"

namespace util::graph
{

template<typename N, typename E>
class Graph
{
public:
    virtual ~Graph()
    {
        for (auto n : _nodes)
            delete n;
    }
    virtual Node<N, E>* addNd() = 0;
    virtual Node<N, E>* addNd(const N& pl) = 0;

    Edge<N, E>* addEdg(Node<N, E>* from, Node<N, E>* to)
    {
        return addEdg(from, to, E());
    }

    virtual Edge<N, E>* addEdg(Node<N, E>* from, Node<N, E>* to, const E& p) = 0;

    Edge<N, E>* getEdg(Node<N, E>* from, Node<N, E>* to)
    {
        for (auto e : from->getAdjList())
        {
            if (e->getOtherNd(from) == to)
                return e;
        }

        return nullptr;
    }
    const Edge<N, E>* getEdg(Node<N, E>* from, Node<N, E>* to) const
    {
        for (auto e : from->getAdjList())
        {
            if (e->getOtherNd(from) == to)
                return e;
        }

        return nullptr;
    }

    virtual Node<N, E>* mergeNds(Node<N, E>* a, Node<N, E>* b) = 0;

    const std::set<Node<N, E>*>& getNds() const
    {
        return _nodes;
    }
    std::set<Node<N, E>*>* getNds()
    {
        return &_nodes;
    }

    typename std::set<Node<N, E>*>::iterator delNd(Node<N, E>* n)
    {
        return delNd(_nodes.find(n));
    }
    typename std::set<Node<N, E>*>::iterator delNd(typename std::set<Node<N, E>*>::iterator i)
    {
        delete *i;
        return _nodes.erase(i);
    }
    void delEdg(Node<N, E>* from, Node<N, E>* to)
    {
        Edge<N, E>* toDel = getEdg(from, to);
        if (!toDel) return;

        from->removeEdge(toDel);
        to->removeEdge(toDel);

        assert(!getEdg(from, to));

        delete toDel;
    }

private:
    std::set<Node<N, E>*> _nodes;
};

}

#endif  // UTIL_GRAPH_GRAPH_H_
