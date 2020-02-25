#ifndef TACO_ATTR_QUERY_NODES_H
#define TACO_ATTR_QUERY_NODES_H

#include <vector>
#include <memory>
#include <string>
#include <utility>

#include "taco/attr_query/attr_query.h"
#include "taco/attr_query/attr_query_visitor.h"
#include "taco/index_notation/index_notation.h"

namespace taco {
namespace attr_query {

struct SelectNode : public AttrQueryNode {
  SelectNode(const std::vector<IndexVarExpr>& groupBys,
             const std::vector<std::pair<AttrQuery,std::string>>& attrs) :
      groupBys(groupBys), attrs(attrs) {}

  void accept(AttrQueryVisitorStrict* v) const {
    v->visit(this);
  }

  std::vector<IndexVarExpr> groupBys;
  std::vector<std::pair<AttrQuery,std::string>> attrs;
};


struct LiteralNode : public AttrQueryNode {
  LiteralNode(int val) : val(val) {}

  void accept(AttrQueryVisitorStrict* v) const {
    v->visit(this);
  }

  int val;
};


struct DistinctCountNode : public AttrQueryNode {
  DistinctCountNode(const std::vector<IndexVarExpr>& coords) : coords(coords) {}

  void accept(AttrQueryVisitorStrict* v) const {
    v->visit(this);
  }

  std::vector<IndexVarExpr> coords;
};


struct MaxNode : public AttrQueryNode {
  MaxNode(IndexVarExpr coord) : coord(coord) {}

  void accept(AttrQueryVisitorStrict* v) const {
    v->visit(this);
  }

  IndexVarExpr coord;
};


struct MinNode : public AttrQueryNode {
  MinNode(IndexVarExpr coord) : coord(coord) {}

  void accept(AttrQueryVisitorStrict* v) const {
    v->visit(this);
  }

  IndexVarExpr coord;
};


/// Returns true if expression e is of type E.
template <typename E>
inline bool isa(const AttrQueryNode* e) {
  return e != nullptr && dynamic_cast<const E*>(e) != nullptr;
}

/// Casts the expression e to type E.
template <typename E>
inline const E* to(const AttrQueryNode* e) {
  taco_iassert(isa<E>(e)) <<
      "Cannot convert " << typeid(e).name() << " to " << typeid(E).name();
  return static_cast<const E*>(e);
}

template <typename I>
inline const typename I::Node* getNode(const I& stmt) {
  taco_iassert(isa<typename I::Node>(stmt.ptr));
  return static_cast<const typename I::Node*>(stmt.ptr);
}

}}
#endif
