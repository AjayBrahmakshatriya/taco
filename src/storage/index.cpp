#include "taco/storage/index.h"

#include <iostream>

#include "taco/format.h"
#include "taco/error.h"
#include "taco/storage/array.h"
#include "taco/storage/array_util.h"

using namespace std;

namespace taco {
namespace storage {

// class Index
struct Index::Content {
  Format format;
  vector<DimensionIndex> indices;
};

Index::Index() : content(new Content) {
}

Index::Index(const Format& format, const std::vector<DimensionIndex>& indices)
    : Index() {
  taco_iassert(format.getOrder() == indices.size()  );
  content->format = format;
  content->indices = indices;
}

const Format& Index::getFormat() const {
  return content->format;
}

size_t Index::numDimensionIndices() const {
  return getFormat().getOrder();
}

const DimensionIndex& Index::getDimensionIndex(int i) const {
  return content->indices[i];
}

DimensionIndex Index::getDimensionIndex(int i) {
  taco_iassert(size_t(i) < getFormat().getOrder());
  return content->indices[i];
}

size_t Index::getSize() const {
  size_t size = 1;
  for (size_t i = 0; i < getFormat().getOrder(); i++) {
    auto dimType  = getFormat().getDimensionTypes()[i];
    auto dimIndex = getDimensionIndex(i);
    switch (dimType) {
      case DimensionType::Dense:
        size *= getValue<size_t>(dimIndex.getIndexArray(0), 0);
        break;
      case DimensionType::Sparse:
        size = getValue<size_t>(dimIndex.getIndexArray(0), size);
        break;
      case DimensionType::Fixed:
        size *= getValue<size_t>(dimIndex.getIndexArray(0), 0);
        break;
      default:
        taco_not_supported_yet;
        break;
    }
  }
  return size;
}

std::ostream& operator<<(std::ostream& os, const Index& index) {
  auto& format = index.getFormat();
  for (size_t i = 0; i < format.getOrder(); i++) {
    os << format.getDimensionTypes()[i] << " ("
       << format.getDimensionOrder()[i] << "): ";
    auto dimIndex = index.getDimensionIndex(i);
    for (size_t j = 0; j < dimIndex.numIndexArrays(); j++) {
      os << endl << "  " << dimIndex.getIndexArray(j);
    }
    if (i < format.getOrder()-1) os << endl;
  }
  return os;
}


// class DimensionIndex
struct DimensionIndex::Content {
  vector<Array> indexArrays;
};

DimensionIndex::DimensionIndex() : content(new Content) {
}

DimensionIndex::DimensionIndex(const std::vector<Array>& indexArrays)
    : DimensionIndex() {
  content->indexArrays = indexArrays;
}

size_t DimensionIndex::numIndexArrays() const {
  return content->indexArrays.size();
}

const Array& DimensionIndex::getIndexArray(int i) const {
  return content->indexArrays[i];
}

Array DimensionIndex::getIndexArray(int i) {
  return content->indexArrays[i];
}

// Factory functions
Index makeCSRIndex(size_t numrows, int* rowptr, int* colidx) {
  return Index(CSR, {DimensionIndex({makeArray({(int)numrows})}),
                     DimensionIndex({makeArray(rowptr, numrows+1),
                                     makeArray(colidx, rowptr[numrows])})});
}

Index makeCSRIndex(const vector<int>& rowptr, const vector<int>& colidx) {
  return Index(CSR, {DimensionIndex({makeArray({(int)(rowptr.size()-1)})}),
                     DimensionIndex({makeArray(rowptr), makeArray(colidx)})});
}

Index makeCSCIndex(size_t numcols, int* colptr, int* rowidx) {
  return Index(CSC, {DimensionIndex({makeArray({(int)numcols})}),
                     DimensionIndex({makeArray(colptr, numcols+1),
                                     makeArray(rowidx, colptr[numcols])})});
}

Index makeCSCIndex(const vector<int>& colptr, const vector<int>& rowidx) {
  return Index(CSC, {DimensionIndex({makeArray({(int)(colptr.size()-1)})}),
                     DimensionIndex({makeArray(colptr), makeArray(rowidx)})});
}

}}
