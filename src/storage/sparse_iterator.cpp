#include "sparse_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

SparseIterator::SparseIterator(std::string name, const Expr& tensor, int level,
                               Iterator previous)
    : IteratorImpl(previous, tensor) {
  this->tensor = tensor;
  this->level = level;

  const std::string prefix = util::toString(tensor) + std::to_string(level);
  ptrVar = Var::make(prefix + "_pos", Type(Type::Int));
  endVar = Var::make(prefix + "_end", Type(Type::Int));
  
  idxVar = Var::make(name + util::toString(tensor), Type(Type::Int));

  posCapacityVar = Var::make(prefix + "_pos_capacity", Type(Type::Int));
  idxCapacityVar = Var::make(prefix + "_idx_capacity", Type(Type::Int));
}

bool SparseIterator::isDense() const {
  return false;
}

bool SparseIterator::isFixedRange() const {
  return false;
}

bool SparseIterator::isRandomAccess() const {
  return false;
}

bool SparseIterator::isSequentialAccess() const {
  return true;
}

bool SparseIterator::hasDuplicates() const {
  return false;
}

Expr SparseIterator::getRangeSize() const {
  return Expr();
}

Expr SparseIterator::getPtrVar() const {
  return ptrVar;
}

Expr SparseIterator::getEndVar() const {
  return endVar;
}

Expr SparseIterator::getIdxVar() const {
  return idxVar;
}

Expr SparseIterator::getIteratorVar() const {
  return ptrVar;
}

Expr SparseIterator::begin() const {
  return Load::make(getPtrArr(), getParent().getPtrVar());
}

Expr SparseIterator::end() const {
  return Load::make(getPtrArr(), getParent().getEndVar());
}

Expr SparseIterator::getIdx(ir::Expr pos) const {
  return Load::make(getIdxArr(), pos);
}

Stmt SparseIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), Load::make(getIdxArr(), getPtrVar()),
                         true);
}

ir::Stmt SparseIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  Expr parentPos = getParent().getPtrVar();
  Stmt storePos = Store::make(getPtrArr(), Add::make(parentPos, 1), ptr);

  if (level == 0) {
    return storePos;
  }

  Expr shouldResize = Lte::make(getPosCapacity(), Add::make(parentPos, 1));
  Expr newCapacity = Mul::make(2, Add::make(parentPos, 1));
  Stmt updateCapacity = VarAssign::make(getPosCapacity(), newCapacity);
  Stmt resizePos = Allocate::make(getPtrArr(), getPosCapacity(), true);
  Stmt body = Block::make({updateCapacity, resizePos});
  Stmt maybeResizePos = IfThenElse::make(shouldResize, body);

  return Block::make({maybeResizePos, storePos});
}

ir::Stmt SparseIterator::storeIdx(ir::Expr idx) const {
  Stmt storeIdx = Store::make(getIdxArr(), getPtrVar(), idx);

  Expr shouldResize = Lte::make(getIdxCapacity(), getPtrVar());
  Expr newCapacity = Mul::make(2, getPtrVar());
  Stmt updateCapacity = VarAssign::make(getIdxCapacity(), newCapacity);
  Stmt resizeIdx = Allocate::make(getIdxArr(), getIdxCapacity(), true);
  Stmt body = Block::make({updateCapacity, resizeIdx});
  Stmt maybeResizeIdx = IfThenElse::make(shouldResize, body);

  return Block::make({maybeResizeIdx, storeIdx});
}

ir::Stmt SparseIterator::initStorage(ir::Expr size) const {
  Stmt initPosCapacity = VarAssign::make(getPosCapacity(), size, true);
  Stmt initIdxCapacity = VarAssign::make(getIdxCapacity(), size, true);
  Stmt allocPosArr = Allocate::make(getPtrArr(), getPosCapacity());
  Stmt allocIdxArr = Allocate::make(getIdxArr(), getIdxCapacity());
  Stmt initPosArr = Store::make(getPtrArr(), 0, 0);

  return Block::make({initPosCapacity, initIdxCapacity, allocPosArr, 
                      allocIdxArr, initPosArr});
}

ir::Expr SparseIterator::getPosCapacity() const {
  return posCapacityVar;
}

ir::Expr SparseIterator::getIdxCapacity() const {
  return idxCapacityVar;
}

ir::Expr SparseIterator::getPtrArr() const {
  return getIndex(0);
}

ir::Expr SparseIterator::getIdxArr() const {
  return getIndex(1);
}

ir::Expr SparseIterator::getIndex(int index) const {
  switch (index) {
    case 0: {
      string name = tensor.as<Var>()->name + to_string(level) + "_pos_arr";
      return GetProperty::make(tensor, TensorProperty::Indices, level, 0, name);
    }
    case 1: {
      string name = tensor.as<Var>()->name + to_string(level) + "_idx_arr";
      return GetProperty::make(tensor, TensorProperty::Indices, level, 1, name);
    }
    default:
      break;
  }
  return Expr();
}

}}
