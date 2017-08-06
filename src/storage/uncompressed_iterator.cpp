#include "uncompressed_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

UncompressedIterator::UncompressedIterator(std::string name, const Expr& tensor, int level,
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

bool UncompressedIterator::isDense() const {
  return false;
}

bool UncompressedIterator::isFixedRange() const {
  return false;
}

bool UncompressedIterator::isRandomAccess() const {
  return false;
}

bool UncompressedIterator::isSequentialAccess() const {
  return true;
}

bool UncompressedIterator::hasDuplicates() const {
  return true;
}

Expr UncompressedIterator::getRangeSize() const {
  return Expr();
}

Expr UncompressedIterator::getPtrVar() const {
  return ptrVar;
}

Expr UncompressedIterator::getEndVar() const {
  return endVar;
}

Expr UncompressedIterator::getIdxVar() const {
  return idxVar;
}

Expr UncompressedIterator::getIteratorVar() const {
  return ptrVar;
}

Expr UncompressedIterator::begin() const {
  return Load::make(getPtrArr(), getParent().getPtrVar());
}

Expr UncompressedIterator::end() const {
  return Load::make(getPtrArr(), getParent().getEndVar());
}

Expr UncompressedIterator::getIdx(ir::Expr pos) const {
  Expr stride = (int)getPack().getSize();
  Expr loc = Mul::make(pos, stride);

  return Load::make(getIdxArr(), loc);
}

Stmt UncompressedIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), getIdx(getPtrVar()), true);
}

ir::Stmt UncompressedIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
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

ir::Stmt UncompressedIterator::storeIdx(ir::Expr idx) const {
  Expr stride = (int)getPack().getSize();
  Expr loc = Mul::make(getPtrVar(), stride);
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

ir::Stmt UncompressedIterator::initStorage(ir::Expr size) const {
  Stmt initPosCapacity = VarAssign::make(getPosCapacity(), size, true);
  Stmt allocPosArr = Allocate::make(getPtrArr(), getPosCapacity());
  Stmt initPosArr = Store::make(getPtrArr(), 0, 0);

  if (getPack().getLastIterator() != this) {
    return Block::make({initPosCapacity, allocPosArr, initPosArr});
  }

  Stmt initIdxCapacity = VarAssign::make(getIdxCapacity(), size, true);
  Stmt allocIdxArr = Allocate::make(getIdxArr(), getIdxCapacity());

  return Block::make({initPosCapacity, initIdxCapacity, allocPosArr, 
                      allocIdxArr, initPosArr});
}

ir::Expr UncompressedIterator::getPosCapacity() const {
  return posCapacityVar;
}

ir::Expr UncompressedIterator::getIdxCapacity() const {
  return idxCapacityVar;
}

ir::Expr UncompressedIterator::getPtrArr() const {
  return getIndex(0);
}

ir::Expr UncompressedIterator::getIdxArr() const {
  return getIndex(1);
}

ir::Expr UncompressedIterator::getIndex(int index) const {
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
      taco_iassert(false);
      break;
  }
  return Expr();
}

}}
