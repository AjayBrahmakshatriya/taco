#include "taco/lower/mode_format_block.h"

using namespace std;
using namespace taco::ir;

namespace taco {

BlockModeFormat::BlockModeFormat(int size) : BlockModeFormat(size, true, true) {
}

BlockModeFormat::BlockModeFormat(int size, const bool isOrdered, const bool isUnique) : 
    ModeFormatImpl("block", true, isOrdered, isUnique, false, true, false,
                   false, true, false, false, false, false, false), size(size) {
}

ModeFormat BlockModeFormat::copy(
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
  return ModeFormat(std::make_shared<BlockModeFormat>(size, isOrdered, isUnique));
}

ModeFunction BlockModeFormat::locate(ir::Expr parentPos,
                                   std::vector<ir::Expr> coords,
                                   Mode mode) const {
  Expr pos = ir::Add::make(
      ir::Mul::make(parentPos, size), 
      ir::Sub::make(coords.back(), ir::Mul::make(size, coords[coords.size() - 3]))
    );
  return ModeFunction(Stmt(), {pos, true});
}

Expr BlockModeFormat::getWidth(Mode mode) const {
  return (mode.getSize().isFixed() && mode.getSize().getSize() < 16) ?
         (int)mode.getSize().getSize() : 
         getSizeArray(mode.getModePack());
}

Expr BlockModeFormat::getSizeNew(Expr prevSize, Mode mode) const {
  return ir::Mul::make(prevSize, ir::Div::make(getWidth(mode), size));
}

ModeFunction BlockModeFormat::getYieldPos(Expr parentPos, 
    std::vector<Expr> coords, Mode mode) const {
  return locate(parentPos, coords, mode);
}

vector<Expr> BlockModeFormat::getArrays(Expr tensor, int mode, 
                                        int level) const {
  return {GetProperty::make(tensor, TensorProperty::Dimension, mode)};
}

Expr BlockModeFormat::getSizeArray(ModePack pack) const {
  return pack.getArray(0);
}

}
