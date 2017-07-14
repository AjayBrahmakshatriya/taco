#include "dense_iterator.h"

#include "taco/util/strings.h"

using namespace taco::ir;

namespace taco {
namespace storage {

DenseIterator::DenseIterator(std::string name, const Expr& tensor, int level,
                             size_t dimSize, Iterator previous)
      : IteratorImpl(previous, tensor) {
  this->tensor = tensor;
  this->level = level;

  std::string indexVarName = name + util::toString(tensor);
  ptrVar = Var::make(util::toString(tensor) + std::to_string(level) + "_pos",
                     Type(Type::Int));
  endVar = Var::make(util::toString(tensor) + std::to_string(level) + "_end",
                     Type(Type::Int));
  idxVar = Var::make(indexVarName, Type(Type::Int));

  this->dimSize = (int)dimSize;
}

bool DenseIterator::isDense() const {
  return true;
}

bool DenseIterator::isFixedRange() const {
  return true;
}

bool DenseIterator::isRandomAccess() const {
  return true;
}

bool DenseIterator::isSequentialAccess() const {
  // TODO: Change to true
  return false;
}

bool DenseIterator::hasDuplicates() const {
  return false;
}

Expr DenseIterator::getRangeSize() const {
  if (isa<Literal>(dimSize) && to<Literal>(dimSize)->value <= 16) {
    return dimSize;
  }
  return getSizeArr();
}

Expr DenseIterator::getPtrVar() const {
  return ptrVar;
}

Expr DenseIterator::getEndVar() const {
  return endVar;
}

Expr DenseIterator::getIdxVar() const {
  return idxVar;
}

Expr DenseIterator::getIteratorVar() const {
  return idxVar;
}

Expr DenseIterator::begin() const {
  return 0;
}

Expr DenseIterator::end() const {
  return getRangeSize();
}

Expr DenseIterator::getIdx(Expr pos) const {
  return Expr();
}

Stmt DenseIterator::initDerivedVars() const {
  Expr ptrVal = Add::make(Mul::make(getParent().getPtrVar(), end()),
                          getIdxVar());
  return VarAssign::make(getPtrVar(), ptrVal);
}

ir::Stmt DenseIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt DenseIterator::storeIdx(ir::Expr idx) const {
  return Stmt();
}

ir::Stmt DenseIterator::initStorage(ir::Expr size) const {
  return Stmt();
}

ir::Expr DenseIterator::getSizeArr() const {
  return GetProperty::make(tensor, TensorProperty::Dimensions, level);
}

}}
