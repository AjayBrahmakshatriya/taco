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
  return Load::make(getIdxArr(), pos);
}

Stmt UncompressedIterator::initDerivedVars() const {
  return VarAssign::make(getIdxVar(), Load::make(getIdxArr(), getPtrVar()),
                         true);
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
  Stmt storeIdx = Store::make(getIdxArr(), getPtrVar(), idx);

  Expr shouldResize = Lte::make(getIdxCapacity(), getPtrVar());
  Expr newCapacity = Mul::make(2, getPtrVar());
  Stmt updateCapacity = VarAssign::make(getIdxCapacity(), newCapacity);
  Stmt resizeIdx = Allocate::make(getIdxArr(), getIdxCapacity(), true);
  Stmt body = Block::make({updateCapacity, resizeIdx});
  Stmt maybeResizeIdx = IfThenElse::make(shouldResize, body);

  return Block::make({maybeResizeIdx, storeIdx});
}

ir::Stmt UncompressedIterator::initStorage(ir::Expr size) const {
  Stmt initPosCapacity = VarAssign::make(getPosCapacity(), size, true);
  Stmt initIdxCapacity = VarAssign::make(getIdxCapacity(), size, true);
  Stmt allocPosArr = Allocate::make(getPtrArr(), getPosCapacity());
  Stmt allocIdxArr = Allocate::make(getIdxArr(), getIdxCapacity());
  Stmt initPosArr = Store::make(getPtrArr(), 0, 0);

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
  string name = tensor.as<Var>()->name + to_string(level) + "_pos_arr";
  return GetProperty::make(tensor, TensorProperty::Indices, level, 0, name);

}

ir::Expr UncompressedIterator::getIdxArr() const {
  string name = tensor.as<Var>()->name + to_string(level) + "_idx_arr";
  return GetProperty::make(tensor, TensorProperty::Indices, level, 1, name);
}

}}
