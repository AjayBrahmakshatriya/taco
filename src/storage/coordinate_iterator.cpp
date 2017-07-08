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

  std::string idxVarName = name + util::toString(tensor);
  ptrVar = Var::make(util::toString(tensor) + std::to_string(level) + "_pos",
                     Type(Type::Int));
  endVar = Var::make(util::toString(tensor) + std::to_string(level) + "_end",
                     Type(Type::Int));
  idxVar = Var::make(idxVarName, Type(Type::Int));
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

RangeType CoordinateIterator::getRangeType() const {
  return RangeType::Single;
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
  return Load::make(getIdxArr(), pos);
}

Stmt CoordinateIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), Load::make(getIdxArr(), getPtrVar()),
                         true);
}

ir::Stmt CoordinateIterator::storePtr() const {
  return Stmt();
}

ir::Stmt CoordinateIterator::storeIdx(ir::Expr idx) const {
  return Store::make(getIdxArr(), getPtrVar(), idx);
}

ir::Expr CoordinateIterator::getIdxArr() const {
  string name = tensor.as<Var>()->name + to_string(level) + "_idx_arr";
  return GetProperty::make(tensor, TensorProperty::Indices, level, 1, name);
}

ir::Stmt CoordinateIterator::initStorage(ir::Expr size) const {
  return Allocate::make(getIdxArr(), size);
}

ir::Stmt CoordinateIterator::resizePtrStorage(ir::Expr size) const {
  return Stmt();
}

ir::Stmt CoordinateIterator::resizeIdxStorage(ir::Expr size) const {
  return Allocate::make(getIdxArr(), size, true);
}

}}
