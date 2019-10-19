#include "taco/attr_query/attr_query_visitor.h"
#include "taco/attr_query/attr_query_nodes.h"

namespace taco {
namespace attr_query {

// class AttrQueryVisitorStrict
AttrQueryVisitorStrict::~AttrQueryVisitorStrict() {
}

void AttrQueryVisitorStrict::visit(const AttrQuery& query) {
  query.accept(this);
}


// class AttrQueryVisitor
AttrQueryVisitor::~AttrQueryVisitor() {
}

void AttrQueryVisitor::visit(const SelectNode* op) {
  for (const auto& attr : op->attrs) {
    attr.first.accept(this);
  }
}

void AttrQueryVisitor::visit(const LiteralNode* op) {
}

void AttrQueryVisitor::visit(const DistinctCountNode* op) {
}

}}
