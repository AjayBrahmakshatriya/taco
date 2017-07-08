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

  std::string idxVarName = name + util::toString(tensor);
  ptrVar = Var::make(util::toString(tensor) + std::to_string(level) + "_pos",
                     Type(Type::Int));
  endVar = Var::make(util::toString(tensor) + std::to_string(level) + "_end",
                     Type(Type::Int));
  idxVar = Var::make(idxVarName, Type(Type::Int));
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

RangeType UncompressedIterator::getRangeType() const {
  return RangeType::Variable;
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

ir::Stmt UncompressedIterator::storePtr() const {
  return Store::make(getPtrArr(),
                     Add::make(getParent().getPtrVar(), 1), getPtrVar());
}

ir::Stmt UncompressedIterator::storeIdx(ir::Expr idx) const {
  return Store::make(getIdxArr(), getPtrVar(), idx);
}

ir::Expr UncompressedIterator::getPtrArr() const {
  string name = tensor.as<Var>()->name + to_string(level) + "_pos_arr";
  return GetProperty::make(tensor, TensorProperty::Indices, level, 0, name);

}

ir::Expr UncompressedIterator::getIdxArr() const {
  string name = tensor.as<Var>()->name + to_string(level) + "_idx_arr";
  return GetProperty::make(tensor, TensorProperty::Indices, level, 1, name);
}

ir::Stmt UncompressedIterator::initStorage(ir::Expr size) const {
  return Block::make({Allocate::make(getPtrArr(), size),
                      Allocate::make(getIdxArr(), size),
                      Store::make(getPtrArr(), 0, 0)});
}

ir::Stmt UncompressedIterator::resizePtrStorage(ir::Expr size) const {
  return Allocate::make(getPtrArr(), size, true);
}

ir::Stmt UncompressedIterator::resizeIdxStorage(ir::Expr size) const {
  return Allocate::make(getIdxArr(), size, true);
}

}}
