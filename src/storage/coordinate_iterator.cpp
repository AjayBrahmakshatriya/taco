#include "coordinate_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

CoordinateIterator::CoordinateIterator(std::string name, const Expr& tensor, int level,
                               Iterator previous)
    : IteratorImpl(previous, tensor) {
  this->tensor = tensor;
  this->level = level;
  
  const std::string prefix = util::toString(tensor) + std::to_string(level);
  ptrVar = Var::make(prefix + "_pos", Type(Type::Int));
  endVar = Var::make(prefix + "_end", Type(Type::Int));
  
  idxVar = Var::make(name + util::toString(tensor), Type(Type::Int));

  idxCapacityVar = Var::make(prefix + "_idx_capacity", Type(Type::Int));
}

bool CoordinateIterator::isDense() const {
  return false;
}

bool CoordinateIterator::isFixedRange() const {
  return false;
}

bool CoordinateIterator::isRandomAccess() const {
  return false;
}

bool CoordinateIterator::isSequentialAccess() const {
  return true;
}

bool CoordinateIterator::hasDuplicates() const {
  return true;
}

Expr CoordinateIterator::getRangeSize() const {
  return 1;
}

Expr CoordinateIterator::getPtrVar() const {
  return ptrVar;
}

Expr CoordinateIterator::getEndVar() const {
  return endVar;
}

Expr CoordinateIterator::getIdxVar() const {
  return idxVar;
}

Expr CoordinateIterator::getIteratorVar() const {
  return ptrVar;
}

Expr CoordinateIterator::begin() const {
  return getParent().getPtrVar();
}

Expr CoordinateIterator::end() const {
  return getParent().getEndVar();
}

Expr CoordinateIterator::getIdx(ir::Expr pos) const {
  Expr stride = (int)getPack().getSize();
  Expr offset = (int)getPack().getPosition(this);
  Expr loc = Add::make(Mul::make(pos, stride), offset);

  return Load::make(getIdxArr(), loc);
}

Stmt CoordinateIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), getIdx(getPtrVar()), true);
}

ir::Stmt CoordinateIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt CoordinateIterator::storeIdx(ir::Expr idx) const {
  Expr stride = (int)getPack().getSize();
  Expr offset = (int)getPack().getPosition(this);
  Expr loc = Add::make(Mul::make(getPtrVar(), stride), offset);
  Stmt storeIdx = Store::make(getIdxArr(), loc, idx);

  if (getPack().getLastIterator() != this) {
    return storeIdx;
  }

  Expr shouldResize = Lte::make(getIdxCapacity(), loc);
  Expr newCapacity = Mul::make(2, loc);
  Stmt updateCapacity = VarAssign::make(getIdxCapacity(), newCapacity);
  Stmt resizeIdx = Allocate::make(getIdxArr(), getIdxCapacity(), true);
  Stmt body = Block::make({updateCapacity, resizeIdx});
  Stmt maybeResizeIdx = IfThenElse::make(shouldResize, body);

  return Block::make({maybeResizeIdx, storeIdx});
}

ir::Stmt CoordinateIterator::initStorage(ir::Expr size) const {
  if (getPack().getLastIterator() != this) {
    return Stmt();
  }

  Stmt initIdxCapacity = VarAssign::make(getIdxCapacity(), size, true);
  Stmt allocIdxArr = Allocate::make(getIdxArr(), getIdxCapacity());

  return Block::make({initIdxCapacity, allocIdxArr});
}

ir::Expr CoordinateIterator::getIdxCapacity() const {
  return idxCapacityVar;
}

ir::Expr CoordinateIterator::getIdxArr() const {
  return getIndex(1);
}

ir::Expr CoordinateIterator::getIndex(int index) const {
  if (getPack().getFirstIterator() != this) {
    return getPack().getFirstIterator().getIndex(index);
  }

  switch (index) {
    case 1: {
      string name = tensor.as<Var>()->name + to_string(level) + "_idx_arr";
      return GetProperty::make(tensor, TensorProperty::Indices, level, 1, name);
    }
    default:
      taco_iassert(false);
      break;
  }
  return Expr();
}

}}
