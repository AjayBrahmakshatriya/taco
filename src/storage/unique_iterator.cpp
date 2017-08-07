#include "unique_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

UniqueIterator::UniqueIterator(std::string name, const Expr& tensor, int level,
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

bool UniqueIterator::isDense() const {
  return false;
}

bool UniqueIterator::isFixedRange() const {
  return false;
}

bool UniqueIterator::isRandomAccess() const {
  return false;
}

bool UniqueIterator::isSequentialAccess() const {
  return true;
}

bool UniqueIterator::hasDuplicates() const {
  return false;
}

Expr UniqueIterator::getRangeSize() const {
  return 1;
}

Expr UniqueIterator::getPtrVar() const {
  return ptrVar;
}

Expr UniqueIterator::getEndVar() const {
  return endVar;
}

Expr UniqueIterator::getIdxVar() const {
  return idxVar;
}

Expr UniqueIterator::getIteratorVar() const {
  return ptrVar;
}

Expr UniqueIterator::begin() const {
  return getParent().getPtrVar();
}

Expr UniqueIterator::end() const {
  return getParent().getEndVar();
}

Expr UniqueIterator::getIdx(ir::Expr pos) const {
  Expr stride = (int)getPack().getSize();
  Expr offset = (int)getPack().getPosition(this);
  Expr loc = Add::make(Mul::make(pos, stride), offset);

  return Load::make(getIdxArr(), loc);
}

Stmt UniqueIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), getIdx(getPtrVar()), true);
}

ir::Stmt UniqueIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt UniqueIterator::storeIdx(ir::Expr idx) const {
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

ir::Stmt UniqueIterator::initStorage(ir::Expr size) const {
  if (getPack().getLastIterator() != this) {
    return Stmt();
  }

  Stmt initIdxCapacity = VarAssign::make(getIdxCapacity(), size, true);
  Stmt allocIdxArr = Allocate::make(getIdxArr(), getIdxCapacity());

  return Block::make({initIdxCapacity, allocIdxArr});
}

ir::Expr UniqueIterator::getIdxCapacity() const {
  return idxCapacityVar;
}

ir::Expr UniqueIterator::getIdxArr() const {
  return getIndex(1);
}

ir::Expr UniqueIterator::getIndex(int index) const {
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
