#include "fixed_iterator.h"

#include "taco/util/strings.h"

using namespace taco::ir;

namespace taco {
namespace storage {

FixedIterator::FixedIterator(std::string name, const Expr& tensor, int level,
                             size_t fixedSize, Iterator previous)
    : IteratorImpl(previous, tensor) {
  this->tensor = tensor;
  this->level = level;

  std::string idxVarName = name + util::toString(tensor);
  ptrVar = Var::make(util::toString(tensor) + std::to_string(level) + "_ptr",
                     Type(Type::Int));
  endVar = Var::make(util::toString(tensor) + std::to_string(level) + "_end",
                     Type(Type::Int));
  idxVar = Var::make(idxVarName,Type(Type::Int));

  this->fixedSize = (int)fixedSize;
}

bool FixedIterator::isDense() const {
  return false;
}

bool FixedIterator::isFixedRange() const {
  return false;
}

bool FixedIterator::isRandomAccess() const {
  return false;
}

bool FixedIterator::isSequentialAccess() const {
  return true;
}

bool FixedIterator::hasDuplicates() const {
  return false;
}

Expr FixedIterator::getRangeSize() const {
  return fixedSize;
}

Expr FixedIterator::getPtrVar() const {
  return ptrVar;
}

Expr FixedIterator::getEndVar() const {
  return endVar;
}

Expr FixedIterator::getIdxVar() const {
  return idxVar;
}

Expr FixedIterator::getIteratorVar() const {
  return ptrVar;
}

Expr FixedIterator::begin() const {
  return 0;
}

Expr FixedIterator::end() const {
  return fixedSize;
}

Expr FixedIterator::getIdx(ir::Expr pos) const {
  return Load::make(getIdxArr(), pos);
}

Stmt FixedIterator::initDerivedVars() const {
  Expr ptrVal = Add::make(Mul::make(getParent().getPtrVar(), end()),
                          getIdxVar());
  return VarAssign::make(getIdxVar(), ptrVal);
}

ir::Stmt FixedIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt FixedIterator::storeIdx(ir::Expr idx) const {
  return Store::make(getIdxArr(), getPtrVar(), idx);
}

ir::Expr FixedIterator::getPtrArr() const {
  return Expr();
}

ir::Expr FixedIterator::getIdxArr() const {
  return Expr();
}

ir::Stmt FixedIterator::initStorage(ir::Expr size) const {
  return Stmt();
}

ir::Expr FixedIterator::getIndex(int index) const {
  return Expr();
}

}}
