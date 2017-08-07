#include "iterators.h"

#include <iostream>
#include <iterator>
#include <algorithm>

#include "taco/tensor.h"
#include "taco/expr.h"
#include "taco/format.h"
#include "taco/ir/ir.h"
#include "taco/error.h"
#include "taco/util/collections.h"

using namespace std;
using namespace taco::ir;
using taco::storage::Iterator;
using taco::storage::IteratorPack;

namespace taco {
namespace lower {

// class Iterators
Iterators::Iterators() {
}

Iterators::Iterators(const IterationSchedule& schedule,
                     const map<TensorBase,ir::Expr>& tensorVariables) {
  // Create an iterator for each path step
  for (auto& path : schedule.getTensorPaths()) {
    TensorBase tensor = path.getTensor();
    Format format = path.getTensor().getFormat();
    ir::Expr tensorVar = tensorVariables.at(tensor);

    storage::Iterator parent = Iterator::makeRoot(tensorVar);
    roots.insert({path, parent});

    const auto& packBoundaries = format.getDimensionPackBoundaries();
    if (!packBoundaries.empty()) {
      const auto& accessOrder = path.getAccessOrder();
      for (size_t i = 0; i < path.getSize(); ++i) {
        const int j = format.getDimensionOrder()[i];
        const auto pos = std::find(accessOrder.begin(), accessOrder.end(), j);
        const auto idx = std::distance(accessOrder.begin(), pos);
        
        const string name = path.getVariables()[idx].getName();
        //std::cout << "name = " << name << std::endl;
        Iterator iterator = Iterator::make(name, tensorVar, i,
                                           format.getDimensionTypes()[idx],
                                           format.getDimensionOrder()[idx],
                                           parent, tensor);
        taco_iassert(path.getStep(idx).getStep() == idx);
        iterators.insert({path.getStep(idx), iterator});

        parent = iterator;
      }

      for (size_t j = 0; j < packBoundaries.size() - 1; ++j) {
        std::vector<Iterator> iteratorPack;
        for (int i = packBoundaries[j]; i < packBoundaries[j + 1]; ++i) {
          const int idx = format.getDimensionOrder()[i];
          iteratorPack.push_back(iterators[path.getStep(idx)]);
        }
        IteratorPack::make(iteratorPack);
      }
    }
  }

  // Create an iterator for the result path
  TensorPath resultPath = schedule.getResultTensorPath();
  if (resultPath.defined()) {
    TensorBase tensor = resultPath.getTensor();
    Format format = tensor.getFormat();
    ir::Expr tensorVar = tensorVariables.at(tensor);

    storage::Iterator parent = Iterator::makeRoot(tensorVar);
    roots.insert({resultPath, parent});

    const auto& packBoundaries = format.getDimensionPackBoundaries();
    if (!packBoundaries.empty()) {
      for (size_t j = 0; j < packBoundaries.size() - 1; ++j) {
        std::vector<Iterator> iteratorPack;
        for (int i = packBoundaries[j]; i < packBoundaries[j + 1]; ++i) {
          string name = resultPath.getVariables()[i].getName();
          Iterator iterator = Iterator::make(name, tensorVar, i,
                                             format.getDimensionTypes()[i],
                                             format.getDimensionOrder()[i],
                                             parent, tensor);
          taco_iassert(resultPath.getStep(i).getStep() == i);
          iteratorPack.push_back(iterator);
          iterators.insert({resultPath.getStep(i), iterator});
          parent = iterator;
        }
        IteratorPack::make(iteratorPack);
      }
    }
  }
}

const storage::Iterator&
Iterators::operator[](const TensorPathStep& step) const {
  taco_iassert(util::contains(iterators, step)) <<
      "No iterator for step: " << step;
  return iterators.at(step);
}

vector<storage::Iterator>
Iterators::operator[](const vector<TensorPathStep>& steps) const {
  vector<storage::Iterator> iterators;
  for (auto& step : steps) {
    iterators.push_back((*this)[step]);
  }
  return iterators;
}

const storage::Iterator& Iterators::getRoot(const TensorPath& path) const {
  taco_iassert(util::contains(roots, path)) <<
      path << " does not have a root iterator";
  return roots.at(path);
}


// functions
bool needsMerge(const std::vector<storage::Iterator>& iterators) {
  int notRandomAccess = 0;
  for (auto& iterator : iterators) {
    if ((!iterator.isRandomAccess()) && (++notRandomAccess > 1)) {
      return true;
    }
  }
  return false;
}

std::vector<storage::Iterator>
getDenseIterators(const std::vector<storage::Iterator>& iterators) {
  vector<storage::Iterator> denseIterators;
  for (auto& iterator : iterators) {
    if (iterator.defined() && iterator.isDense()) {
      denseIterators.push_back(iterator);
    }
  }
  return denseIterators;
}

vector<storage::Iterator>
getSequentialAccessIterators(const vector<storage::Iterator>& iterators) {
  vector<storage::Iterator> sequentialAccessIterators;
  for (auto& iterator : iterators) {
    if (iterator.defined() && iterator.isSequentialAccess()) {
      sequentialAccessIterators.push_back(iterator);
    }
  }
  return sequentialAccessIterators;
}

vector<storage::Iterator>
getRandomAccessIterators(const vector<storage::Iterator>& iterators) {
  vector<storage::Iterator> randomAccessIterators;
  for (auto& iterator : iterators) {
    if (iterator.defined() && iterator.isRandomAccess()) {
      randomAccessIterators.push_back(iterator);
    }
  }
  return randomAccessIterators;
}

vector<ir::Expr> getIdxVars(const vector<storage::Iterator>& iterators) {
  vector<ir::Expr> idxVars;
  for (auto& iterator : iterators) {
    idxVars.push_back(iterator.getIdxVar());
  }
  return idxVars;
}

}}
