#include "iterator.h"

#include "root_iterator.h"
#include "dense_iterator.h"
#include "sparse_iterator.h"
#include "fixed_iterator.h"
#include "uncompressed_iterator.h"
#include "coordinate_iterator.h"
#include "unique_iterator.h"
#include "banded_iterator.h"
#include "offset_iterator.h"

#include "taco/tensor.h"
#include "taco/expr.h"
#include "taco/ir/ir.h"
#include "taco/storage/storage.h"
#include "taco/storage/array.h"
#include "taco/storage/array_util.h"
#include "taco/util/strings.h"

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
      iterator.iterator =
          std::make_shared<UncompressedIterator>(name, tensorVar, dim, parent);
      break;
    }
    case DimensionType::Coordinate: {
      iterator.iterator =
          std::make_shared<CoordinateIterator>(name, tensorVar, dim, parent);
      break;
    }
    case DimensionType::Unique: {
      iterator.iterator =
          std::make_shared<UniqueIterator>(name, tensorVar, dim, parent);
      break;
    }
    case DimensionType::Banded: {
      size_t dimSize = tensor.getDimensions()[dimOrder];
      iterator.iterator =
          std::make_shared<BandedIterator>(name, tensorVar, dim, dimSize,
                                          parent);
      break;
    }
    case DimensionType::Offset: {
      iterator.iterator =
          std::make_shared<OffsetIterator>(name, tensorVar, dim, parent);
      break;
    }
  }
  taco_iassert(iterator.defined());
  return iterator;
}

const IteratorPack& Iterator::getPack() const {
  taco_iassert(defined());
  return iterator->getPack();
}

const Iterator& Iterator::getParent() const {
  taco_iassert(defined());
  return iterator->getParent();
}

ir::Expr Iterator::getTensor() const {
  taco_iassert(defined());
  return iterator->getTensor();
}

bool Iterator::isDense() const {
  taco_iassert(defined());
  return iterator->isDense();
}

bool Iterator::isFixedRange() const {
  taco_iassert(defined());
  return getRangeSize().defined();
}

bool Iterator::isOnlyChild() const {
  taco_iassert(defined());
  return isFixedRange() && ir::isa<ir::Literal>(getRangeSize()) &&
         getRangeSize().as<ir::Literal>()->value == 1;
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

void Iterator::setPack(IteratorPack pack) {
  taco_iassert(defined());
  return iterator->setPack(pack); 
}

bool operator==(const Iterator& a, const IteratorImpl* b) {
  return a.iterator.get() == b;
}

bool operator!=(const Iterator& a, const IteratorImpl* b) {
  return !(a == b);
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


// class IteratorPack
struct IteratorPack::Content {
  Content(const std::vector<Iterator> iterators) : iterators(iterators) {}
  
  std::vector<Iterator> iterators;
};

IteratorPack::IteratorPack() {
}

IteratorPack IteratorPack::make(std::vector<Iterator> iterators) {
  IteratorPack pack;
  pack.content = std::make_shared<Content>(iterators);

  for (auto& iterator : iterators) {
    iterator.setPack(pack);
  }

  return pack;
}

size_t IteratorPack::getSize() const {
  return content->iterators.size();
}

Iterator IteratorPack::getFirstIterator() const {
  return content->iterators.front();
}

Iterator IteratorPack::getLastIterator() const {
  return content->iterators.back();
}

size_t IteratorPack::getPosition(const IteratorImpl* iter) const {
  for (size_t i = 0; i < content->iterators.size(); ++i) {
    if (content->iterators[i] == iter) {
      return i;
    }
  }
  taco_iassert(false);
  return 0;
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

const IteratorPack& IteratorImpl::getPack() const {
  return pack;
}

const Iterator& IteratorImpl::getParent() const {
  return parent;
}

const ir::Expr& IteratorImpl::getTensor() const {
  return tensor;
}

void IteratorImpl::setPack(IteratorPack pack) {
  this->pack = pack;
}

std::ostream& operator<<(std::ostream& os, const IteratorImpl& iterator) {
  return os << iterator.getName();
}

}}
