#ifndef TACO_STORAGE_UNCOMPRESSED_AOS_H
#define TACO_STORAGE_UNCOMPRESSED_AOS_H

#include <string>

#include "iterator.h"
#include "taco/ir/ir.h"

namespace taco {
namespace storage {

class UncompressedAosIterator : public IteratorImpl {
public:
  UncompressedAosIterator(std::string name, const ir::Expr& tensor, int level,
                 Iterator previous);
  virtual ~UncompressedAosIterator() {};

  bool isDense() const;
  bool isFixedRange() const;

  bool isRandomAccess() const;
  bool isSequentialAccess() const;

  bool hasDuplicates() const;

  ir::Expr getRangeSize() const;

  ir::Expr getPtrVar() const;
  ir::Expr getEndVar() const;
  ir::Expr getIdxVar() const;

  ir::Expr getIteratorVar() const;
  ir::Expr begin() const;
  ir::Expr end() const;

  ir::Expr getIdx(ir::Expr pos) const;

  ir::Stmt initDerivedVars() const;

  ir::Stmt storePtr(ir::Expr ptr, ir::Expr start) const;
  ir::Stmt storeIdx(ir::Expr idx) const;

  ir::Stmt initStorage(ir::Expr size) const;

  ir::Expr getIndex(int index) const;

private:
  ir::Expr tensor;
  int level;

  ir::Expr posCapacityVar;
  ir::Expr idxCapacityVar;

  ir::Expr ptrVar;
  ir::Expr endVar;
  ir::Expr idxVar;

  ir::Expr getPosCapacity() const;
  ir::Expr getIdxCapacity() const;

  ir::Expr getPtrArr() const;
  ir::Expr getIdxArr() const;
};

}}
#endif
