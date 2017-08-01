#include "unique_aos_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

UniqueAosIterator::UniqueAosIterator(std::string name, const Expr& tensor, int level,
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

bool UniqueAosIterator::isDense() const {
  return false;
}

bool UniqueAosIterator::isFixedRange() const {
  return false;
}

bool UniqueAosIterator::isRandomAccess() const {
  return false;
}

bool UniqueAosIterator::isSequentialAccess() const {
  return true;
}

bool UniqueAosIterator::hasDuplicates() const {
  return false;
}

Expr UniqueAosIterator::getRangeSize() const {
  return 1;
}

Expr UniqueAosIterator::getPtrVar() const {
  return ptrVar;
}

Expr UniqueAosIterator::getEndVar() const {
  return endVar;
}

Expr UniqueAosIterator::getIdxVar() const {
  return idxVar;
}

Expr UniqueAosIterator::getIteratorVar() const {
  return ptrVar;
}

Expr UniqueAosIterator::begin() const {
  return getParent().getPtrVar();
}

Expr UniqueAosIterator::end() const {
  return getParent().getEndVar();
}

Expr UniqueAosIterator::getIdx(ir::Expr pos) const {
  Expr order = GetProperty::make(tensor, TensorProperty::Order);
  Expr loc = Add::make(Mul::make(pos, order), level);

  return Load::make(getIdxArr(), loc);
}

Stmt UniqueAosIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), getIdx(getPtrVar()), true);
}

ir::Stmt UniqueAosIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt UniqueAosIterator::storeIdx(ir::Expr idx) const {
  Expr order = GetProperty::make(tensor, TensorProperty::Order);
  Expr loc = Add::make(Mul::make(getPtrVar(), order), level);
  Stmt storeIdx = Store::make(getIdxArr(), loc, idx);

  
  Expr shouldResize = Lte::make(getIdxCapacity(), loc);
  Expr newCapacity = Mul::make(2, loc);
  Stmt updateCapacity = VarAssign::make(getIdxCapacity(), newCapacity);
  Stmt resizeIdx = Allocate::make(getIdxArr(), getIdxCapacity(), true);
  Stmt body = Block::make({updateCapacity, resizeIdx});
  Stmt maybeResizeIdx = IfThenElse::make(shouldResize, body);

  return Block::make({maybeResizeIdx, storeIdx});
}

ir::Stmt UniqueAosIterator::initStorage(ir::Expr size) const {
  Stmt initIdxCapacity = VarAssign::make(getIdxCapacity(), size, true);
  Stmt allocIdxArr = Allocate::make(getIdxArr(), getIdxCapacity());

  return Block::make({initIdxCapacity, allocIdxArr});
}

ir::Expr UniqueAosIterator::getIdxCapacity() const {
  return idxCapacityVar;
}

ir::Expr UniqueAosIterator::getIdxArr() const {
  return getIndex(1);
}

ir::Expr UniqueAosIterator::getIndex(int index) const {
  return getParent().getIndex(index);
}

}}
