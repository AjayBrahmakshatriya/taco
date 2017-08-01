#include "uncompressed_aos_iterator.h"

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {
namespace storage {

UncompressedAosIterator::UncompressedAosIterator(std::string name, const Expr& tensor, int level,
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

bool UncompressedAosIterator::isDense() const {
  return false;
}

bool UncompressedAosIterator::isFixedRange() const {
  return false;
}

bool UncompressedAosIterator::isRandomAccess() const {
  return false;
}

bool UncompressedAosIterator::isSequentialAccess() const {
  return true;
}

bool UncompressedAosIterator::hasDuplicates() const {
  return true;
}

Expr UncompressedAosIterator::getRangeSize() const {
  return Expr();
}

Expr UncompressedAosIterator::getPtrVar() const {
  return ptrVar;
}

Expr UncompressedAosIterator::getEndVar() const {
  return endVar;
}

Expr UncompressedAosIterator::getIdxVar() const {
  return idxVar;
}

Expr UncompressedAosIterator::getIteratorVar() const {
  return ptrVar;
}

Expr UncompressedAosIterator::begin() const {
  return Load::make(getPtrArr(), getParent().getPtrVar());
}

Expr UncompressedAosIterator::end() const {
  return Load::make(getPtrArr(), getParent().getEndVar());
}

Expr UncompressedAosIterator::getIdx(ir::Expr pos) const {
  Expr order = GetProperty::make(tensor, TensorProperty::Order);
  Expr loc = Add::make(Mul::make(pos, order), level);

  return Load::make(getIdxArr(), loc);
}

Stmt UncompressedAosIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), getIdx(getPtrVar()), true);
}

ir::Stmt UncompressedAosIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
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

ir::Stmt UncompressedAosIterator::storeIdx(ir::Expr idx) const {
  Expr order = GetProperty::make(tensor, TensorProperty::Order);
  Expr loc = Add::make(Mul::make(getPtrVar(), order), level);
  
  return Store::make(getIdxArr(), loc, idx);
}

ir::Stmt UncompressedAosIterator::initStorage(ir::Expr size) const {
  Stmt initPosCapacity = VarAssign::make(getPosCapacity(), size, true);
  Stmt allocPosArr = Allocate::make(getPtrArr(), getPosCapacity());
  Stmt initPosArr = Store::make(getPtrArr(), 0, 0);

  return Block::make({initPosCapacity, allocPosArr, initPosArr});
}

ir::Expr UncompressedAosIterator::getPosCapacity() const {
  return posCapacityVar;
}

ir::Expr UncompressedAosIterator::getIdxCapacity() const {
  return idxCapacityVar;
}

ir::Expr UncompressedAosIterator::getPtrArr() const {
  return getIndex(0);
}

ir::Expr UncompressedAosIterator::getIdxArr() const {
  return getIndex(1);
}

ir::Expr UncompressedAosIterator::getIndex(int index) const {
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
