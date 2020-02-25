#ifndef TACO_ATTR_QUERY_H
#define TACO_ATTR_QUERY_H

#include <ostream>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <utility>

#include "taco/index_notation/index_notation.h"
#include "taco/util/uncopyable.h"
#include "taco/util/intrusive_ptr.h"

namespace taco {
namespace attr_query {

struct SelectNode;
struct LiteralNode;
struct DistinctCountNode;
struct MaxNode;
struct MinNode;

class AttrQueryVisitorStrict;

struct AttrQueryNode : public util::Manageable<AttrQueryNode>,
                       private util::Uncopyable {
public:
  AttrQueryNode() = default;
  virtual ~AttrQueryNode() = default;
  virtual void accept(AttrQueryVisitorStrict*) const = 0;
};


class AttrQuery : public util::IntrusivePtr<const AttrQueryNode> {
public:
  AttrQuery() : util::IntrusivePtr<const AttrQueryNode>(nullptr) {}
  AttrQuery(const AttrQueryNode* n) : util::IntrusivePtr<const AttrQueryNode>(n) {}

  AttrQuery(int lit);

  /// Visit the attributs query's sub-expressions.
  void accept(AttrQueryVisitorStrict *) const;

  /// Print the attribute query.
  friend std::ostream& operator<<(std::ostream&, const AttrQuery&);
};

/// Print the attr query expr.
std::ostream& operator<<(std::ostream&, const AttrQuery&);

/// Return true if the attr query expr is of the given subtype.
template <typename SubType> bool isa(AttrQuery);

/// Casts the attr query expr to the given subtype. 
template <typename SubType> SubType to(AttrQuery);


class Select : public AttrQuery {
public:
  Select() = default;
  Select(const SelectNode*);
  Select(const std::vector<IndexVarExpr>& groupBys,
         const std::pair<AttrQuery,std::string>& attr);
  Select(const std::vector<IndexVarExpr>& groupBys,
         const std::vector<std::pair<AttrQuery,std::string>>& attrs);

  const std::vector<IndexVarExpr>& getGroupBys() const;
  const std::vector<std::pair<AttrQuery,std::string>>& getAttrs() const;

  typedef SelectNode Node;
};


class Literal : public AttrQuery {
public:
  Literal() = default;
  Literal(const LiteralNode*);
  Literal(int val);

  /// Returns the literal value.
  int getVal() const;

  typedef LiteralNode Node;
};


class DistinctCount : public AttrQuery {
public:
  DistinctCount() = default;
  DistinctCount(const DistinctCountNode*);
  DistinctCount(IndexVarExpr coord);
  DistinctCount(const std::vector<IndexVarExpr>& coord);

  const std::vector<IndexVarExpr>& getCoords() const;

  typedef DistinctCountNode Node;
};


class Max : public AttrQuery {
public:
  Max();
  Max(const MaxNode*);
  Max(IndexVarExpr coord);

  IndexVarExpr getCoord() const;

  typedef MaxNode Node;
};


class Min : public AttrQuery {
public:
  Min();
  Min(const MinNode*);
  Min(IndexVarExpr coord);

  IndexVarExpr getCoord() const;

  typedef MinNode Node;
};


#if 0
class Count : public AttrQuery {
public:
  Count();
  Count(const CountNode*);
  Count(const std::vector<>& indexVars);

  const std::vector<>& gets() const;

  typedef CountNode Node;
};
#endif

}}
#endif
