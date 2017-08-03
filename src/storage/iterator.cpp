#include "iterator.h"

#include "root_iterator.h"
#include "dense_iterator.h"
#include "sparse_iterator.h"
#include "fixed_iterator.h"
#include "uncompressed_iterator.h"
#include "coordinate_iterator.h"
#include "unique_iterator.h"
#include "uncompressed_aos_iterator.h"
#include "coordinate_aos_iterator.h"
#include "unique_aos_iterator.h"

#include "taco/tensor.h"
#include "taco/expr.h"
#include "taco/ir/ir.h"
#include "taco/storage/storage.h"
#include "taco/storage/array.h"
#include "taco/storage/array_util.h"
#include "taco/util/strings.h"

#define AOS 0

using namespace std;

namespace taco {
namespace storage {

// class Iterator
Iterator::Iterator() : iterator(nullptr) {
}

Iterator Iterator::makeRoot(const ir::Expr& tensor) {
  Iterator iterator;
  iterator.iterator = std::make_shared<RootIterator>(tensor);
  return iterator;
}

Iterator Iterator::make(string name, const ir::Expr& tensorVar,
                        int dim, DimensionType dimType, int dimOrder,
                        Iterator parent, const TensorBase& tensor) {
  Iterator iterator;

  switch (dimType) {
    case DimensionType::Dense: {
      size_t dimSize = tensor.getDimensions()[dimOrder];
      iterator.iterator =
          std::make_shared<DenseIterator>(name, tensorVar, dim, dimSize,
                                          parent);
      break;
    }
    case DimensionType::Sparse: {
      iterator.iterator =
          std::make_shared<SparseIterator>(name, tensorVar, dim, parent);
      break;
    }
    case DimensionType::Fixed: {
      auto dimIndex = tensor.getStorage().getIndex().getDimensionIndex(dim);
      int fixedSize = getValue<int>(dimIndex.getIndexArray(0), 0);
      iterator.iterator =
          std::make_shared<FixedIterator>(name, tensorVar, dim, fixedSize,
                                          parent);
      break;
    }
    case DimensionType::Uncompressed: {
#if AOS
      iterator.iterator =
          std::make_shared<UncompressedAosIterator>(name, tensorVar, dim, parent);
#else
      iterator.iterator =
          std::make_shared<UncompressedIterator>(name, tensorVar, dim, parent);
#endif
      break;
    }
    case DimensionType::Coordinate: {
#if AOS
      iterator.iterator =
          std::make_shared<CoordinateAosIterator>(name, tensorVar, dim, parent);
#else
      iterator.iterator =
          std::make_shared<CoordinateIterator>(name, tensorVar, dim, parent);
#endif
      break;
    }
    case DimensionType::Unique: {
#if AOS
      iterator.iterator =
          std::make_shared<UniqueAosIterator>(name, tensorVar, dim, parent);
#else
      iterator.iterator =
          std::make_shared<UniqueIterator>(name, tensorVar, dim, parent);
#endif
      break;
    }
  }
  taco_iassert(iterator.defined());
  return iterator;
}

const Iterator& Iterator::getParent() const {
  taco_iassert(defined());
  return iterator->getParent();
}

bool Iterator::isDense() const {
  taco_iassert(defined());
  return iterator->isDense();
}

bool Iterator::isFixedRange() const {
  taco_iassert(defined());
  return getRangeSize().defined();
}

bool Iterator::isRandomAccess() const {
  taco_iassert(defined());
  return iterator->isRandomAccess();
}

bool Iterator::isSequentialAccess() const {
  taco_iassert(defined());
  return iterator->isSequentialAccess();
}

bool Iterator::hasDuplicates() const {
  taco_iassert(defined());
  return iterator->hasDuplicates();
}

ir::Expr Iterator::getRangeSize() const {
  taco_iassert(defined());
  return iterator->getRangeSize();
}

ir::Expr Iterator::getTensor() const {
  taco_iassert(defined());
  return iterator->getTensor();
}

ir::Expr Iterator::getPtrVar() const {
  taco_iassert(defined());
  return iterator->getPtrVar();
}

ir::Expr Iterator::getEndVar() const {
  taco_iassert(defined());
  return iterator->getEndVar();
}

ir::Expr Iterator::getIdxVar() const {
  taco_iassert(defined());
  return iterator->getIdxVar();
}

ir::Expr Iterator::getIteratorVar() const {
  taco_iassert(defined());
  return iterator->getIteratorVar();
}

ir::Expr Iterator::begin() const {
  taco_iassert(defined());
  return iterator->begin();
}

ir::Expr Iterator::end() const {
  taco_iassert(defined());
  return iterator->end();
}

ir::Expr Iterator::getIdx(ir::Expr pos) const {
  taco_iassert(defined());
  return iterator->getIdx(pos);
}

ir::Stmt Iterator::initDerivedVar() const {
  taco_iassert(defined());
  return iterator->initDerivedVars();
}

ir::Stmt Iterator::storePtr(ir::Expr ptr, ir::Expr start) const {
  taco_iassert(defined());
  return iterator->storePtr(ptr, start);
}

ir::Stmt Iterator::storeIdx(ir::Expr idx) const {
  taco_iassert(defined());
  return iterator->storeIdx(idx);
}

ir::Stmt Iterator::initStorage(ir::Expr size) const {
  taco_iassert(defined());
  return iterator->initStorage(size);
}

ir::Expr Iterator::getIndex(int index) const {
  taco_iassert(defined());
  return iterator->getIndex(index);
}

bool Iterator::defined() const {
  return iterator != nullptr;
}

bool operator==(const Iterator& a, const Iterator& b) {
  return a.iterator == b.iterator;
}

bool operator<(const Iterator& a, const Iterator& b) {
  return a.iterator < b.iterator;
}

std::ostream& operator<<(std::ostream& os, const Iterator& iterator) {
  if (!iterator.defined()) {
    return os << "Iterator()";
  }
  return os << *iterator.iterator;
}


// class IteratorImpl
IteratorImpl::IteratorImpl(Iterator parent, ir::Expr tensor) :
    parent(parent), tensor(tensor) {
}

IteratorImpl::~IteratorImpl() {
}

std::string IteratorImpl::getName() const {
  return util::toString(tensor);
}

const Iterator& IteratorImpl::getParent() const {
  return parent;
}

const ir::Expr& IteratorImpl::getTensor() const {
  return tensor;
}

std::ostream& operator<<(std::ostream& os, const IteratorImpl& iterator) {
  return os << iterator.getName();
}

}}
