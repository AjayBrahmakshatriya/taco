#include "taco/attr_query/attr_query_printer.h"

#include <string>

#include "taco/attr_query/attr_query_nodes.h"
#include "taco/util/strings.h"

namespace taco {
namespace attr_query {

AttrQueryPrinter::AttrQueryPrinter(std::ostream& os) : os(os) {
}

void AttrQueryPrinter::print(const AttrQuery& expr) {
  expr.accept(this);
}

void AttrQueryPrinter::visit(const SelectNode* op) {
  os << "select " << util::join(op->groupBys, ", ") << " -> ";
  std::string sep = "";
  for (const auto& attr : op->attrs) {
    os << sep;
    attr.first.accept(this);
    os << " as " << attr.second;
    sep = ", ";
  }
}

void AttrQueryPrinter::visit(const LiteralNode* op) {
  os << op->val;
}

void AttrQueryPrinter::visit(const DistinctCountNode* op) {
  os << "distinct_count(" << op->coord << ")";
}

void AttrQueryPrinter::visit(const MaxNode* op) {
  os << "max(" << op->coord << ")";
}

void AttrQueryPrinter::visit(const MinNode* op) {
  os << "min(" << op->coord << ")";
}

}}
