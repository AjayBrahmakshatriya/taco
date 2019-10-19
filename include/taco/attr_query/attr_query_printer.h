#ifndef TACO_ATTR_QUERY_PRINTER_H
#define TACO_ATTR_QUERY_PRINTER_H

#include <ostream>
#include "taco/attr_query/attr_query_visitor.h"

namespace taco {
namespace attr_query {

class AttrQueryPrinter : public AttrQueryVisitorStrict {
public:
  AttrQueryPrinter(std::ostream& os);

  void print(const AttrQuery& expr);

  using AttrQueryVisitorStrict::visit;

  void visit(const SelectNode* node);
  void visit(const LiteralNode* node);
  void visit(const DistinctCountNode* node);

private:
  std::ostream& os;
};

}}
#endif
