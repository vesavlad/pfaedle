// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GRAPH_UNDIRNODE_H_
#define UTIL_GRAPH_UNDIRNODE_H_

#include <vector>
#include <algorithm>
#include "util/graph/Node.h"

namespace util {
namespace graph {

template <typename N, typename E>
class UndirNode : public Node<N, E> {
 public:
  UndirNode();
  UndirNode(const N& pl);
  ~UndirNode() override;

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
  std::vector<Edge<N, E>*> _adjList;
  N _pl;

  bool adjContains(const Edge<N, E>* e) const;
};

#include "util/graph/UndirNode.tpp"

}}

#endif  // UTIL_GRAPH_UNDIRNODE_H_
