// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GRAPH_UNDIRNODE_H_
#define UTIL_GRAPH_UNDIRNODE_H_

#include <vector>
#include <algorithm>
#include "util/graph/Node.h"

namespace util::graph
{

template<typename N, typename E>
class UndirNode : public Node<N, E>
{
public:
    UndirNode() :
        _pl()
    {
    }
    UndirNode(const N& pl) :
        _pl(pl)
    {
    }
    ~UndirNode() override
    {
        // delete self edges
        for (auto e = _adjList.begin(); e != _adjList.end();)
        {
            Edge<N, E>* eP = *e;
            if (eP->getTo() == this && eP->getFrom() == this)
            {
                e = _adjList.erase(e);
                delete eP;
            }
            else
            {
                e++;
            }
        }

        for (auto e = _adjList.begin(); e != _adjList.end(); e++)
        {
            Edge<N, E>* eP = *e;

            if (eP->getTo() != this)
            {
                eP->getTo()->removeEdge(eP);
            }

            if (eP->getFrom() != this)
            {
                eP->getFrom()->removeEdge(eP);
            }

            delete eP;
        }
    }

    const std::vector<Edge<N, E>*>& getAdjList() const override
    {
        return _adjList;
    }
    const std::vector<Edge<N, E>*>& getAdjListIn() const override
    {
        return _adjList;
    }
    const std::vector<Edge<N, E>*>& getAdjListOut() const override
    {
        return _adjList;
    }

    size_t getDeg() const override
    {
        return _adjList.size();
    }
    size_t getInDeg() const override
    {
        return getDeg();
    }
    size_t getOutDeg() const override
    {
        return getDeg();
    }

    bool hasEdgeIn(const Edge<N, E>* e) const override
    {
        return hasEdge(e);
    }

    bool hasEdgeOut(const Edge<N, E>* e) const override
    {
        return hasEdge(e);
    }
    bool hasEdge(const Edge<N, E>* e) const override
    {
        return e->getFrom() == this || e->getTo() == this;
    }

    // add edge to this node's adjacency lists
    void addEdge(Edge<N, E>* e) override
    {
        if (adjContains(e)) return;
        _adjList.reserve(_adjList.size() + 1);
        _adjList.push_back(e);
    }

    // remove edge from this node's adjacency lists
    void removeEdge(Edge<N, E>* e) override
    {
        auto p = std::find(_adjList.begin(), _adjList.end(), e);
        if (p != _adjList.end()) _adjList.erase(p);
    }

    N& pl() override
    {
        return _pl;
    }
    const N& pl() const override
    {
        return _pl;
    }

private:
    bool adjContains(const Edge<N, E>* e) const
    {
        for (size_t i = 0; i < _adjList.size(); i++)
            if (_adjList[i] == e) return true;
        return false;
    }

    std::vector<Edge<N, E>*> _adjList;
    N _pl;
};

}

#endif  // UTIL_GRAPH_UNDIRNODE_H_
