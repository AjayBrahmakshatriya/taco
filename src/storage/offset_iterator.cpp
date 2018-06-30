#include "offset_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

OffsetIterator::OffsetIterator(std::string name, const Expr& tensor, int level,
                               Iterator previous)
    : IteratorImpl(previous, tensor) {
  this->tensor = tensor;
  this->level = level;

  const std::string prefix = util::toString(tensor) + std::to_string(level);
  ptrVar = Var::make(prefix + "_pos", Type(Type::Int));
  endVar = Var::make(prefix + "_end", Type(Type::Int));
  
  idxVar = Var::make(name + util::toString(tensor), Type(Type::Int));
}

bool OffsetIterator::isDense() const {
  return false;
}

bool OffsetIterator::isFixedRange() const {
  return false;
}

bool OffsetIterator::isRandomAccess() const {
  return false;
}

bool OffsetIterator::isSequentialAccess() const {
  return true;
}

bool OffsetIterator::hasDuplicates() const {
  return false;
}

Expr OffsetIterator::getRangeSize() const {
  return 1;
}

Expr OffsetIterator::getPtrVar() const {
  return ptrVar;
}

Expr OffsetIterator::getEndVar() const {
  return endVar;
}

Expr OffsetIterator::getIdxVar() const {
  return idxVar;
}

Expr OffsetIterator::getIteratorVar() const {
  return ptrVar;
}

Expr OffsetIterator::begin() const {
  return getParent().getPtrVar();
}

Expr OffsetIterator::end() const {
  return getParent().getEndVar();
}

Expr OffsetIterator::getIdx(ir::Expr pos) const {
  return Add::make(getParent().getIdxVar(), Load::make(getOffsetArr(), getParent().getParent().getIdxVar()));
}

Stmt OffsetIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), getIdx(getPtrVar()), true);
}

ir::Stmt OffsetIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt OffsetIterator::storeIdx(ir::Expr idx) const {
  return Stmt();
}

ir::Stmt OffsetIterator::initStorage(ir::Expr size) const {
  return Stmt();
}

ir::Expr OffsetIterator::getOffsetArr() const {
  return getIndex(0);
}

ir::Expr OffsetIterator::getIndex(int index) const {
  if (getPack().getFirstIterator() != this) {
    return getPack().getFirstIterator().getIndex(index);
  }

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

}}
