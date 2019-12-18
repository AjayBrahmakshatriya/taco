#ifndef TACO_ATTR_QUERY_VISITOR_H
#define TACO_ATTR_QUERY_VISITOR_H

#include <vector>
#include <functional>
#include "taco/error.h"

namespace taco {
namespace attr_query {

class AttrQuery;

struct SelectNode;
struct LiteralNode;
struct DistinctCountNode;
struct MaxNode;
struct MinNode;

class AttrQueryVisitorStrict {
public:
  virtual ~AttrQueryVisitorStrict();

  void visit(const AttrQuery&);

  virtual void visit(const SelectNode*) = 0;
  virtual void visit(const LiteralNode*) = 0;
  virtual void visit(const DistinctCountNode*) = 0;
  virtual void visit(const MaxNode*) = 0;
  virtual void visit(const MinNode*) = 0;
};

/// Visit nodes in an attribute query.
class AttrQueryVisitor : public AttrQueryVisitorStrict {
public:
  virtual ~AttrQueryVisitor();

  using AttrQueryVisitorStrict::visit;

  virtual void visit(const SelectNode* node);
  virtual void visit(const LiteralNode* node);
  virtual void visit(const DistinctCountNode* node);
  virtual void visit(const MaxNode*);
  virtual void visit(const MinNode*);
};

}}
#endif
