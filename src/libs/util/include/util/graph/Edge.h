// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GRAPH_EDGE_H_
#define UTIL_GRAPH_EDGE_H_

#include <vector>
#include "util/graph/Node.h"

namespace util::graph
{

template<typename N, typename E>
class Edge
{
public:
    Edge(Node<N, E>* from, Node<N, E>* to, const E& pl);

    Node<N, E>* getFrom() const;
    Node<N, E>* getTo() const;

    Node<N, E>* getOtherNd(const Node<N, E>* notNode) const;

    void setFrom(Node<N, E>* from);
    void setTo(Node<N, E>* to);

    E& pl();
    const E& pl() const;

private:
    Node<N, E>* _from;
    Node<N, E>* _to;
    E _pl;
};

// _____________________________________________________________________________
template<typename N, typename E>
Edge<N, E>::Edge(Node<N, E>* from, Node<N, E>* to, const E& pl) :
    _from(from), _to(to), _pl(pl)
{
}

// _____________________________________________________________________________
template<typename N, typename E>
Node<N, E>* Edge<N, E>::getFrom() const
{
    return _from;
}

// _____________________________________________________________________________
template<typename N, typename E>
Node<N, E>* Edge<N, E>::getTo() const
{
    return _to;
}

// _____________________________________________________________________________
template<typename N, typename E>
Node<N, E>* Edge<N, E>::getOtherNd(const Node<N, E>* notNode) const
{
    if (_to == notNode) return _from;
    return _to;
}

// _____________________________________________________________________________
template<typename N, typename E>
E& Edge<N, E>::pl()
{
    return _pl;
}

// _____________________________________________________________________________
template<typename N, typename E>
const E& Edge<N, E>::pl() const
{
    return _pl;
}

}

#endif  // UTIL_GRAPH_EDGE_H_

