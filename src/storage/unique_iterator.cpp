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

  std::string idxVarName = name + util::toString(tensor);
  ptrVar = Var::make(util::toString(tensor) + std::to_string(level) + "_pos",
                     Type(Type::Int));
  endVar = Var::make(util::toString(tensor) + std::to_string(level) + "_end",
                     Type(Type::Int));
  idxVar = Var::make(idxVarName, Type(Type::Int));
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

RangeType UniqueIterator::getRangeType() const {
  return RangeType::Single;
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
  return Load::make(getIdxArr(), pos);
}

Stmt UniqueIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), Load::make(getIdxArr(), getPtrVar()),
                         true);
}

ir::Stmt UniqueIterator::storePtr() const {
  return Stmt();
}

ir::Stmt UniqueIterator::storeIdx(ir::Expr idx) const {
  return Store::make(getIdxArr(), getPtrVar(), idx);
}

ir::Expr UniqueIterator::getIdxArr() const {
  string name = tensor.as<Var>()->name + to_string(level) + "_idx_arr";
  return GetProperty::make(tensor, TensorProperty::Indices, level, 1, name);
}

ir::Stmt UniqueIterator::initStorage(ir::Expr size) const {
  return Allocate::make(getIdxArr(), size);
}

ir::Stmt UniqueIterator::resizePtrStorage(ir::Expr size) const {
  return Stmt();
}

ir::Stmt UniqueIterator::resizeIdxStorage(ir::Expr size) const {
  return Allocate::make(getIdxArr(), size, true);
}

}}
