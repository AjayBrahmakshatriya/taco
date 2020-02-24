#include "taco/lower/mode_format_sliced.h"

using namespace std;
using namespace taco::ir;

#include "taco/util/strings.h"

namespace taco {

SlicedModeFormat::SlicedModeFormat() : SlicedModeFormat(true, true) {
}

SlicedModeFormat::SlicedModeFormat(const bool isOrdered, const bool isUnique) : 
    ModeFormatImpl("sliced", false, isOrdered, isUnique, false, true, true,
                   false, true, false, false, false, false, false) {
}

ModeFormat SlicedModeFormat::copy(
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
  return ModeFormat(std::make_shared<SlicedModeFormat>(isOrdered, isUnique));
}

std::vector<attr_query::AttrQuery>
SlicedModeFormat::attrQueries(std::vector<IndexVarExpr> coords, 
                              std::vector<IndexVarExpr> vals) const {
  const auto maxQuery = attr_query::Select({}, 
      std::make_pair(attr_query::Max(coords.back()), "max_coord"));
  return {maxQuery};
}

ModeFunction SlicedModeFormat::locate(ir::Expr parentPos,
                                   std::vector<ir::Expr> coords,
                                   Mode mode) const {
  Expr pos = ir::Add::make(ir::Mul::make(parentPos, getWidth(mode)), coords.back());
  return ModeFunction(Stmt(), {pos, ir::Lt::make(coords.back(), getWidth(mode))});
}

Stmt SlicedModeFormat::getInsertCoord(Expr p, 
    const std::vector<Expr>& i, Mode mode) const {
  return Stmt();
}

Expr SlicedModeFormat::getWidth(Mode mode) const {
  return Load::make(getUBArray(mode.getModePack()), 0);
}

Stmt SlicedModeFormat::getInsertInitCoords(Expr pBegin, 
    Expr pEnd, Mode mode) const {
  return Stmt();
}

Stmt SlicedModeFormat::getInsertInitLevel(Expr szPrev, Expr sz, 
    Mode mode) const {
  return Stmt();
}

Stmt SlicedModeFormat::getInsertFinalizeLevel(Expr szPrev, 
    Expr sz, Mode mode) const {
  return Stmt();
}

Expr SlicedModeFormat::getSizeNew(Expr prevSize, Mode mode) const {
  return ir::Mul::make(prevSize, getWidth(mode));
}

Stmt SlicedModeFormat::getInitCoords(Expr prevSize, 
    std::map<std::string,AttrQueryResult> queries, Mode mode) const {
  Expr maxCoord = queries["max_coord"].getResult({}, "max_coord");
  Expr ub = getUBArray(mode.getModePack());
  return Block::make({Allocate::make(ub, 1), Store::make(ub, 0, maxCoord)});
}

ModeFunction SlicedModeFormat::getYieldPos(Expr parentPos, 
    std::vector<Expr> coords, Mode mode) const {
  return locate(parentPos, coords, mode);
}

vector<Expr> SlicedModeFormat::getArrays(Expr tensor, int mode, 
                                        int level) const {
  std::string arraysName = util::toString(tensor) + std::to_string(level);
  return {GetProperty::make(tensor, TensorProperty::Indices,
                            level - 1, 0, arraysName + "_ub")};
}

Expr SlicedModeFormat::getUBArray(ModePack pack) const {
  return pack.getArray(0);
}

}
