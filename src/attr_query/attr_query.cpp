#include "taco/attr_query/attr_query.h"

#include <string>
#include <utility>
#include <vector>

#include "taco/attr_query/attr_query_nodes.h"
#include "taco/attr_query/attr_query_visitor.h"
#include "taco/attr_query/attr_query_printer.h"
#include "taco/index_notation/index_notation.h"

namespace taco {
namespace attr_query {

void AttrQuery::accept(AttrQueryVisitorStrict *v) const {
  ptr->accept(v);
}

AttrQuery::AttrQuery(int val) : AttrQuery(new LiteralNode(val)) {
}

std::ostream& operator<<(std::ostream& os, const AttrQuery& query) {
  if (!query.defined()) return os << "AttrQuery()";
  AttrQueryPrinter printer(os);
  printer.print(query);
  return os;
}


// class Select
Select::Select(const SelectNode* n) : AttrQuery(n) {
}

Select::Select(const std::vector<IndexVarExpr>& groupBys,
               const std::pair<AttrQuery,std::string>& attr)
    : Select(new SelectNode(groupBys, {attr})) {
}

Select::Select(const std::vector<IndexVarExpr>& groupBys,
               const std::vector<std::pair<AttrQuery,std::string>>& attrs)
    : Select(new SelectNode(groupBys, attrs)) {
}

const std::vector<IndexVarExpr>& Select::getGroupBys() const {
  return getNode(*this)->groupBys;
}

const std::vector<std::pair<AttrQuery,std::string>>& Select::getAttrs() const {
  return getNode(*this)->attrs;
}

template <> bool isa<Select>(AttrQuery s) {
  return isa<SelectNode>(s.ptr);
}

template <> Select to<Select>(AttrQuery s) {
  taco_iassert(isa<Select>(s));
  return Select(to<SelectNode>(s.ptr));
}


// class Literal
Literal::Literal(const LiteralNode* n) : AttrQuery(n) {
}

Literal::Literal(int val)
    : Literal(new LiteralNode(val)) {
}

int Literal::getVal() const {
  return getNode(*this)->val;
}

template <> bool isa<Literal>(AttrQuery s) {
  return isa<LiteralNode>(s.ptr);
}

template <> Literal to<Literal>(AttrQuery s) {
  taco_iassert(isa<Literal>(s));
  return Literal(to<LiteralNode>(s.ptr));
}


// class DistinctCount
DistinctCount::DistinctCount(const DistinctCountNode* n) : AttrQuery(n) {
}

DistinctCount::DistinctCount(IndexVarExpr coord)
    : DistinctCount(new DistinctCountNode({coord})) {
}

DistinctCount::DistinctCount(const std::vector<IndexVarExpr>& coords)
    : DistinctCount(new DistinctCountNode(coords)) {
}

const std::vector<IndexVarExpr>& DistinctCount::getCoords() const {
  return getNode(*this)->coords;
}

template <> bool isa<DistinctCount>(AttrQuery s) {
  return isa<DistinctCountNode>(s.ptr);
}

template <> DistinctCount to<DistinctCount>(AttrQuery s) {
  taco_iassert(isa<DistinctCount>(s));
  return DistinctCount(to<DistinctCountNode>(s.ptr));
}


// class Max
Max::Max(const MaxNode* n) : AttrQuery(n) {
}

Max::Max(IndexVarExpr coord)
    : Max(new MaxNode(coord)) {
}

IndexVarExpr Max::getCoord() const {
  return getNode(*this)->coord;
}

template <> bool isa<Max>(AttrQuery s) {
  return isa<MaxNode>(s.ptr);
}

template <> Max to<Max>(AttrQuery s) {
  taco_iassert(isa<Max>(s));
  return Max(to<MaxNode>(s.ptr));
}


// class Min
Min::Min(const MinNode* n) : AttrQuery(n) {
}

Min::Min(IndexVarExpr coord)
    : Min(new MinNode(coord)) {
}

IndexVarExpr Min::getCoord() const {
  return getNode(*this)->coord;
}

template <> bool isa<Min>(AttrQuery s) {
  return isa<MinNode>(s.ptr);
}

template <> Min to<Min>(AttrQuery s) {
  taco_iassert(isa<Min>(s));
  return Min(to<MinNode>(s.ptr));
}

}}
