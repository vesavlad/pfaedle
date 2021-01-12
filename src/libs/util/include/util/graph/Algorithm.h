// Copyright 2017, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GRAPH_ALGORITHM_H_
#define UTIL_GRAPH_ALGORITHM_H_

#include <stack>
#include "util/graph/Edge.h"
#include "util/graph/UndirGraph.h"
#include "util/graph/Node.h"

namespace util::graph
{

// collection of general graph algorithms
class Algorithm
{
public:
    template<typename N, typename E>
    static std::vector<std::set<Node<N, E>*>> connectedComponents(const UndirGraph<N, E>& g)
    {
        std::vector<std::set<Node<N, E>*>> ret;
        std::set<Node<N, E>*> visited;

        for (auto* n : g.getNds())
        {
            if (!visited.count(n))
            {
                ret.resize(ret.size() + 1);
                std::stack<Node<N, E>*> q;
                q.push(n);
                while (!q.empty())
                {
                    Node<N, E>* cur = q.top();
                    q.pop();

                    ret.back().insert(cur);
                    visited.insert(cur);

                    for (auto* e : cur->getAdjList())
                    {
                        if (!visited.count(e->getOtherNd(cur))) q.push(e->getOtherNd(cur));
                    }
                }
            }
        }

        return ret;
    }
};

}

#endif  // UTIL_GRAPH_ALGORITHM_H_
