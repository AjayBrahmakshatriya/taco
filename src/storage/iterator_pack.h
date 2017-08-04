#ifndef TACO_STORAGE_ITERATOR_PACK_H
#define TACO_STORAGE_ITERATOR_PACK_H

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "iterator.h"
#include "taco/ir/ir.h"
#include "taco/util/comparable.h"

namespace taco {

class IteratorPack {
public:
  

private:
  std::vector<Iterator> iterators;
};

}
#endif
