#include "banded_iterator.h"

#include "taco/util/strings.h"

using namespace taco::ir;

namespace taco {
namespace storage {

BandedIterator::BandedIterator(std::string name, const Expr& tensor, int level,
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

bool BandedIterator::isDense() const {
  return false;
}

bool BandedIterator::isFixedRange() const {
  return true;
}

bool BandedIterator::isRandomAccess() const {
  return true;
}

bool BandedIterator::isSequentialAccess() const {
  // TODO: Change to true
  return false;
}

bool BandedIterator::hasDuplicates() const {
  return false;
}

Expr BandedIterator::getRangeSize() const {
  if (isa<Literal>(dimSize) && to<Literal>(dimSize)->value <= 16) {
    return dimSize;
  }
  return getSizeArr();
}

Expr BandedIterator::getPtrVar() const {
  return ptrVar;
}

Expr BandedIterator::getEndVar() const {
  return endVar;
}

Expr BandedIterator::getIdxVar() const {
  return idxVar;
}

Expr BandedIterator::getIteratorVar() const {
  return idxVar;
}

Expr BandedIterator::begin() const {
  return Max::make(0, Neg::make(Load::make(getOffsetArr(), getParent().getIdxVar())));
}

Expr BandedIterator::end() const {
  taco_iassert(getPack().getFirstIterator() == this);
  taco_iassert(getPack().getSize() > 1);

  return Min::make(getRangeSize(), 
      Sub::make(GetProperty::make(tensor, TensorProperty::Dimensions, level+1),
      Load::make(getOffsetArr(), getParent().getIdxVar())));
}

Expr BandedIterator::getIdx(Expr pos) const {
  return Expr();
}

Stmt BandedIterator::initDerivedVars() const {
  Expr ptrVal = Add::make(Mul::make(getParent().getPtrVar(), getRangeSize()),
                          getIdxVar());
  return VarAssign::make(getPtrVar(), ptrVal, true);
}

ir::Stmt BandedIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt BandedIterator::storeIdx(ir::Expr idx) const {
  return Stmt();
}

ir::Stmt BandedIterator::initStorage(ir::Expr size) const {
  return Stmt();
}

ir::Expr BandedIterator::getIndex(int index) const {
  switch (index) {
    case 0: {
      auto name = tensor.as<Var>()->name + std::to_string(level) + "_offset";
      return GetProperty::make(tensor, TensorProperty::Indices, level, 0, name);
    }
    default:
      taco_iassert(false);
      break;
  }
  return Expr();
}

ir::Expr BandedIterator::getSizeArr() const {
  return GetProperty::make(tensor, TensorProperty::Dimensions, level);
}

ir::Expr BandedIterator::getOffsetArr() const {
  const auto name = tensor.as<Var>()->name + std::to_string(level) + "_offset";
  return GetProperty::make(tensor, TensorProperty::Indices, level, 0, name);
}

}}
