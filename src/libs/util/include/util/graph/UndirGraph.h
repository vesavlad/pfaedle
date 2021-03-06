// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GRAPH_UNDIRGRAPH_H_
#define UTIL_GRAPH_UNDIRGRAPH_H_

#include <set>
#include <string>

#include "util/graph/Graph.h"
#include "util/graph/Edge.h"
#include "util/graph/UndirNode.h"

namespace util::graph
{

template<typename N, typename E>
using UndirEdge = Edge<N, E>;

template<typename N, typename E>
class UndirGraph : public Graph<N, E>
{
public:
    UndirGraph() = default;

    using Graph<N, E>::addEdg;

    Node<N, E>* addNd() override
    {
        return addNd(new UndirNode<N, E>());
    }
    Node<N, E>* addNd(UndirNode<N, E>* n)
    {
        auto ins = Graph<N, E>::getNds().insert(n);
        return *ins.first;
    }

    Node<N, E>* addNd(const N& pl) override
    {
        return addNd(new UndirNode<N, E>(pl));
    }

    Edge<N, E>* addEdg(Node<N, E>* from, Node<N, E>* to, const E& p) override
    {
        Edge<N, E>* e = Graph<N, E>::getEdg(from, to);
        if (!e)
        {
            e = new Edge<N, E>(from, to, p);
            from->addEdge(e);
            to->addEdge(e);
        }
        return e;
    }

    Node<N, E>* mergeNds(Node<N, E>* a, Node<N, E>* b) override
    {
        for (auto e : a->getAdjListOut())
        {
            if (e->getTo() != b)
            {
                addEdg(b, e->getTo(), e->pl());
            }
        }

        for (auto e : a->getAdjListIn())
        {
            if (e->getFrom() != b)
            {
                addEdg(e->getFrom(), b, e->pl());
            }
        }

        UndirGraph<N, E>::delNd(a);

        return b;
    }
};

}

#endif  // UTIL_GRAPH_UNDIRGRAPH_H_
