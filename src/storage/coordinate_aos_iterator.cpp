#include "coordinate_aos_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

CoordinateAosIterator::CoordinateAosIterator(std::string name, const Expr& tensor, int level,
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

bool CoordinateAosIterator::isDense() const {
  return false;
}

bool CoordinateAosIterator::isFixedRange() const {
  return false;
}

bool CoordinateAosIterator::isRandomAccess() const {
  return false;
}

bool CoordinateAosIterator::isSequentialAccess() const {
  return true;
}

bool CoordinateAosIterator::hasDuplicates() const {
  return true;
}

Expr CoordinateAosIterator::getRangeSize() const {
  return 1;
}

Expr CoordinateAosIterator::getPtrVar() const {
  return ptrVar;
}

Expr CoordinateAosIterator::getEndVar() const {
  return endVar;
}

Expr CoordinateAosIterator::getIdxVar() const {
  return idxVar;
}

Expr CoordinateAosIterator::getIteratorVar() const {
  return ptrVar;
}

Expr CoordinateAosIterator::begin() const {
  return getParent().getPtrVar();
}

Expr CoordinateAosIterator::end() const {
  return getParent().getEndVar();
}

Expr CoordinateAosIterator::getIdx(ir::Expr pos) const {
  Expr order = GetProperty::make(tensor, TensorProperty::Order);
  Expr loc = Add::make(Mul::make(pos, order), level);

  return Load::make(getIdxArr(), loc);
}

Stmt CoordinateAosIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), getIdx(getPtrVar()), true);
}

ir::Stmt CoordinateAosIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt CoordinateAosIterator::storeIdx(ir::Expr idx) const {
  Expr order = GetProperty::make(tensor, TensorProperty::Order);
  Expr loc = Add::make(Mul::make(getPtrVar(), order), level);
  
  return Store::make(getIdxArr(), loc, idx);
}

ir::Stmt CoordinateAosIterator::initStorage(ir::Expr size) const {
  return Stmt();
}

ir::Expr CoordinateAosIterator::getIdxCapacity() const {
  return idxCapacityVar;
}

ir::Expr CoordinateAosIterator::getIdxArr() const {
  return getIndex(1);
}

ir::Expr CoordinateAosIterator::getIndex(int index) const {
  return getParent().getIndex(index);
}

}}
