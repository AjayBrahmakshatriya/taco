#include "taco/format.h"

#include <iostream>

#include "taco/error.h"
#include "taco/util/strings.h"

namespace taco {

// class Format
Format::Format() {
}

Format::Format(const DimensionType& dimensionType) {
  this->dimensionTypes.push_back(dimensionType);
  this->dimensionOrder.push_back(0);
  this->dimensionPackBoundaries.push_back(0);
  this->dimensionPackBoundaries.push_back(1);
}

Format::Format(const std::vector<DimensionType>& dimensionTypes) {
  this->dimensionTypes = dimensionTypes;
  this->dimensionOrder.resize(dimensionTypes.size());
  this->dimensionPackBoundaries.resize(dimensionTypes.size() + 1);
  this->dimensionPackBoundaries[0] = 0;
  for (size_t i=0; i < dimensionTypes.size(); ++i) {
    this->dimensionOrder[i] = i;
    this->dimensionPackBoundaries[i + 1] = i + 1;
  }
}

Format::Format(const std::vector<DimensionType>& dimensionTypes,
               const std::vector<int>& dimensionOrder) {
  taco_uassert(dimensionTypes.size() == dimensionOrder.size()) <<
      "You must either provide a complete dimension ordering or none";
  this->dimensionTypes = dimensionTypes;
  this->dimensionOrder = dimensionOrder;
  this->dimensionPackBoundaries.resize(dimensionTypes.size() + 1);
  this->dimensionPackBoundaries[0] = 0;
  for (size_t i = 0; i < dimensionTypes.size(); ++i) {
    this->dimensionPackBoundaries[i + 1] = i + 1;
  }
}

Format::Format(const std::vector<DimensionType>& dimensionTypes,
               const std::vector<int>& dimensionOrder,
               const std::vector<int>& dimensionPackBoundaries) {
  taco_uassert(dimensionTypes.size() == dimensionOrder.size()) <<
      "You must either provide a complete dimension ordering or none";
  this->dimensionTypes = dimensionTypes;
  this->dimensionOrder = dimensionOrder;
  this->dimensionPackBoundaries = dimensionPackBoundaries;
}

size_t Format::getOrder() const {
  taco_iassert(this->dimensionTypes.size() == this->getDimensionOrder().size());
  return this->dimensionTypes.size();
}

const std::vector<DimensionType>& Format::getDimensionTypes() const {
  return this->dimensionTypes;
}

const std::vector<int>& Format::getDimensionOrder() const {
  return this->dimensionOrder;
}

const std::vector<int>& Format::getDimensionPackBoundaries() const {
  return this->dimensionPackBoundaries;
}

bool operator==(const Format& a, const Format& b){
  auto& aDimTypes = a.getDimensionTypes();
  auto& bDimTypes = b.getDimensionTypes();
  auto& aDimOrder = a.getDimensionOrder();
  auto& bDimOrder = b.getDimensionOrder();
  auto& aDimPackBoundaries = a.getDimensionPackBoundaries();
  auto& bDimPackBoundaries = b.getDimensionPackBoundaries();
  if (aDimTypes.size() == bDimTypes.size() && 
      aDimPackBoundaries.size() == bDimPackBoundaries.size()) {
    for (size_t i = 0; i < aDimTypes.size(); i++) {
      if ((aDimTypes[i] != bDimTypes[i]) || (aDimOrder[i] != bDimOrder[i])) {
        return false;
      }
    }
    for (size_t i = 0; i < aDimPackBoundaries.size(); i++) {
      if (aDimPackBoundaries[i] != bDimPackBoundaries[i]) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool operator!=(const Format& a, const Format& b) {
  return !(a == b);
}

std::ostream &operator<<(std::ostream& os, const Format& format) {
  return os << "(" << util::join(format.getDimensionTypes(), ",") << "; "
            << util::join(format.getDimensionOrder(), ",") << ")";
}

std::ostream& operator<<(std::ostream& os, const DimensionType& dimensionType) {
  switch (dimensionType) {
    case DimensionType::Dense:
      os << "dense";
      break;
    case DimensionType::Sparse:
      os << "sparse";
      break;
    case DimensionType::Fixed:
      os << "fixed";
      break;
    case DimensionType::Uncompressed:
      os << "uncompressed";
      break;
    case DimensionType::Coordinate:
      os << "coordinate";
      break;
    case DimensionType::Unique:
      os << "unique";
      break;
    default:
      taco_not_supported_yet;
      break;
  }
  return os;
}

// Predefined formats
const Format CSR({Dense, Sparse}, {0,1});
const Format CSC({Dense, Sparse}, {1,0});
const Format DCSR({Sparse, Sparse}, {0,1});
const Format DCSC({Sparse, Sparse}, {1,0});

bool isDense(const Format& format) {
  for (DimensionType dimensionType : format.getDimensionTypes()) {
    if (dimensionType != Dense) {
      return false;
    }
  }
  return true;
}

}
