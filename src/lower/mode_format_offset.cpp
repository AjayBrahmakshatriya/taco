#include "taco/lower/mode_format_offset.h"

using namespace std;
using namespace taco::ir;

namespace taco {

OffsetModeFormat::OffsetModeFormat() : OffsetModeFormat(true, true) {
}

OffsetModeFormat::OffsetModeFormat(const bool isOrdered, const bool isUnique) : 
    ModeFormatImpl("offset", true, isOrdered, isUnique, false, true, false,
                   false, false, false, false, false, false, false) {
}

ModeFormat OffsetModeFormat::copy(
    std::vector<ModeFormat::Property> properties) const {
  bool isOrdered = this->isOrdered;
  bool isUnique = this->isUnique;
  for (const auto property : properties) {
    switch (property) {
      case ModeFormat::ORDERED:
        isOrdered = true;
        break;
      case ModeFormat::NOT_ORDERED:
        isOrdered = false;
        break;
      case ModeFormat::UNIQUE:
        isUnique = true;
        break;
      case ModeFormat::NOT_UNIQUE:
        isUnique = false;
        break;
      default:
        break;
    }
  }
  return ModeFormat(std::make_shared<OffsetModeFormat>(isOrdered, isUnique));
}

Expr OffsetModeFormat::getSizeNew(Expr prevSize, Mode mode) const {
  return prevSize;
}

ModeFunction OffsetModeFormat::getYieldPos(Expr parentPos, 
    std::vector<Expr> coords, Mode mode) const {
  return ModeFunction(Stmt(), {parentPos});
}

std::vector<Expr> OffsetModeFormat::getArrays(Expr tensor, int mode, 
                                              int level) const {
  return {};
}

}
