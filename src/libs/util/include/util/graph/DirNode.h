// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GRAPH_DIRNODE_H_
#define UTIL_GRAPH_DIRNODE_H_

#include <vector>
#include <algorithm>
#include "util/graph/Node.h"

namespace util::graph
{

// forward declaration of Edge
template<typename N, typename E>
class DirNode : public Node<N, E>
{
public:
    DirNode();
    DirNode(const N& pl);
    ~DirNode() override;

    const std::vector<Edge<N, E>*>& getAdjList() const override;
    const std::vector<Edge<N, E>*>& getAdjListIn() const override;
    const std::vector<Edge<N, E>*>& getAdjListOut() const override;

    size_t getDeg() const override;
    size_t getInDeg() const override;
    size_t getOutDeg() const override;

    bool hasEdgeIn(const Edge<N, E>* e) const override;
    bool hasEdgeOut(const Edge<N, E>* e) const override;
    bool hasEdge(const Edge<N, E>* e) const override;

    // add edge to this node's adjacency lists
    void addEdge(Edge<N, E>* e) override;

    // remove edge from this node's adjacency lists
    void removeEdge(Edge<N, E>* e) override;

    N& pl() override;
    const N& pl() const override;

private:
    std::vector<Edge<N, E>*> _adjListIn;
    std::vector<Edge<N, E>*> _adjListOut;
    N _pl;

    bool adjInContains(const Edge<N, E>* e) const;
    bool adjOutContains(const Edge<N, E>* e) const;
};

template<typename N, typename E>
DirNode<N, E>::DirNode() :
    _pl()
{}

template<typename N, typename E>
DirNode<N, E>::DirNode(const N& pl) :
    _pl(pl)
{}

template<typename N, typename E>
DirNode<N, E>::~DirNode()
{
    // delete self edges
    for (auto e = _adjListOut.begin(); e != _adjListOut.end();)
    {
        Edge<N, E>* eP = *e;
        if (eP->getTo() == this)
        {
            _adjListIn.erase(std::find(_adjListIn.begin(), _adjListIn.end(), eP));
            e = _adjListOut.erase(e);
            delete eP;
        }
        else
        {
            e++;
        }
    }

    for (auto e = _adjListOut.begin(); e != _adjListOut.end(); e++)
    {
        Edge<N, E>* eP = *e;

        if (eP->getTo() != this)
        {
            eP->getTo()->removeEdge(eP);
            delete eP;
        }
    }

    for (auto e = _adjListIn.begin(); e != _adjListIn.end(); e++)
    {
        Edge<N, E>* eP = *e;

        if (eP->getFrom() != this)
        {
            eP->getFrom()->removeEdge(eP);
            delete eP;
        }
    }
}

template<typename N, typename E>
void DirNode<N, E>::addEdge(Edge<N, E>* e)
{
    if (e->getFrom() == this && !adjOutContains(e))
    {
        _adjListOut.reserve(_adjListOut.size() + 1);
        _adjListOut.push_back(e);
    }
    if (e->getTo() == this && !adjInContains(e))
    {
        _adjListIn.reserve(_adjListIn.size() + 1);
        _adjListIn.push_back(e);
    }
}

template<typename N, typename E>
void DirNode<N, E>::removeEdge(Edge<N, E>* e)
{
    if (e->getFrom() == this)
    {
        auto p = std::find(_adjListOut.begin(), _adjListOut.end(), e);
        if (p != _adjListOut.end()) _adjListOut.erase(p);
    }
    if (e->getTo() == this)
    {
        auto p = std::find(_adjListIn.begin(), _adjListIn.end(), e);
        if (p != _adjListIn.end()) _adjListIn.erase(p);
    }
}
//
template<typename N, typename E>
bool DirNode<N, E>::hasEdgeIn(const Edge<N, E>* e) const
{
    return e->getTo() == this;
}

template<typename N, typename E>
bool DirNode<N, E>::hasEdgeOut(const Edge<N, E>* e) const
{
    return e->getFrom() == this;
}

template<typename N, typename E>
bool DirNode<N, E>::hasEdge(const Edge<N, E>* e) const
{
    return hasEdgeOut(e) || hasEdgeIn(e);
}

template<typename N, typename E>
const std::vector<Edge<N, E>*>& DirNode<N, E>::getAdjList() const
{
    return _adjListOut;
}

template<typename N, typename E>
const std::vector<Edge<N, E>*>& DirNode<N, E>::getAdjListOut() const
{
    return _adjListOut;
}

template<typename N, typename E>
const std::vector<Edge<N, E>*>& DirNode<N, E>::getAdjListIn() const
{
    return _adjListIn;
}

template<typename N, typename E>
size_t DirNode<N, E>::getDeg() const
{
    return _adjListOut.size();
}

template<typename N, typename E>
size_t DirNode<N, E>::getInDeg() const
{
    return _adjListIn.size();
}

template<typename N, typename E>
size_t DirNode<N, E>::getOutDeg() const
{
    return _adjListOut.size();
}

template<typename N, typename E>
N& DirNode<N, E>::pl()
{
    return _pl;
}

template<typename N, typename E>
const N& DirNode<N, E>::pl() const
{
    return _pl;
}

template<typename N, typename E>
bool DirNode<N, E>::adjInContains(const Edge<N, E>* e) const
{
    for (size_t i = 0; i < _adjListIn.size(); i++)
        if (_adjListIn[i] == e) return true;
    return false;
}

template<typename N, typename E>
bool DirNode<N, E>::adjOutContains(const Edge<N, E>* e) const
{
    for (size_t i = 0; i < _adjListOut.size(); i++)
        if (_adjListOut[i] == e) return true;
    return false;
}


}

#endif  // UTIL_GRAPH_DIRNODE_H_
