#include "root_iterator.h"

using namespace taco::ir;

namespace taco {
namespace storage {

RootIterator::RootIterator(const ir::Expr& tensor)
    : IteratorImpl(Iterator(), tensor) {
}

bool RootIterator::isDense() const {
  return true;
}

bool RootIterator::isFixedRange() const {
  return true;
}

bool RootIterator::isRandomAccess() const {
  return false;
}

bool RootIterator::isSequentialAccess() const {
  return true;
}

bool RootIterator::hasDuplicates() const {
  return false;
}

Expr RootIterator::getRangeSize() const {
  return 1;
}

Expr RootIterator::getPtrVar() const {
  return 0;
}

Expr RootIterator::getEndVar() const {
  return 1;
}

Expr RootIterator::getIdxVar() const {
  taco_ierror << "The root iterator does not have an index var";
  return Expr();
}

ir::Expr RootIterator::getIteratorVar() const {
  taco_ierror << "The root node does not have an iterator variable";
  return Expr();
}

Expr RootIterator::begin() const {
  taco_ierror << "The root node does not have an iterator variable";
  return 0;
}

Expr RootIterator::end() const {
  taco_ierror << "The root node does not have an iterator variable";
  return 1;
}

Expr RootIterator::getIdx(ir::Expr pos) const {
  return Expr();
}

ir::Stmt RootIterator::initDerivedVars() const {
  return Stmt();
}

ir::Stmt RootIterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  return Stmt();
}

ir::Stmt RootIterator::storeIdx(ir::Expr idx) const {
  return Stmt();
}

ir::Stmt RootIterator::initStorage(ir::Expr size) const {
  return Stmt();
}

}}
