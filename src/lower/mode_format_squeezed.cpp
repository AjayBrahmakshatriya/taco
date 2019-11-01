#include "taco/lower/mode_format_squeezed.h"

#include <vector>
#include <string>

#include "taco/util/strings.h"

using namespace std;
using namespace taco::ir;

namespace taco {

SqueezedModeFormat::SqueezedModeFormat() : SqueezedModeFormat(true, true) {
}

SqueezedModeFormat::SqueezedModeFormat(const bool isOrdered, 
                                       const bool isUnique) : 
    ModeFormatImpl("squeezed", true, isOrdered, isUnique, false, true, false,
                   false, true, false, false, false, false, false) {
}

ModeFormat SqueezedModeFormat::copy(
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
  return ModeFormat(std::make_shared<SqueezedModeFormat>(isOrdered, isUnique));
}

std::vector<attr_query::AttrQuery>
SqueezedModeFormat::attrQueries(std::vector<IndexVarExpr> coords) const {
  return {attr_query::Select({coords.back()}, 
      std::make_pair(attr_query::Literal(1), "nonempty"))};
}

Expr SqueezedModeFormat::getSizeNew(Expr prevSize, Mode mode) const {
  return ir::Mul::make(prevSize, Load::make(getPermSizeArray(mode.getModePack()), 0));
}

Stmt SqueezedModeFormat::getInitCoords(Expr prevSize, 
    std::map<std::string,AttrQueryResult> queries, Mode mode) const {
  Expr permArray = getPermArray(mode.getModePack());
  Expr permSizeArray = getPermSizeArray(mode.getModePack());
  Stmt allocPerm = Allocate::make(permArray, getSizeArray(mode.getModePack()));
  Stmt initPermSize = Store::make(permSizeArray, 0, 0);
  Expr idx = Var::make("i", Int());
  Expr loadPermSize = Load::make(permSizeArray, 0);
  Stmt storePerm = Store::make(permArray, loadPermSize, idx);
  Stmt incPermSize = Store::make(permSizeArray, 0, ir::Add::make(loadPermSize, 1));
  Expr isNonempty = Eq::make(queries["nonempty"].getResult({idx}, "nonempty"), 1);
  Stmt maybeStorePerm = IfThenElse::make(isNonempty, Block::make(storePerm, incPermSize));
  Stmt storePerms = For::make(idx, 0, getSizeArray(mode.getModePack()), 1, maybeStorePerm);
  return Block::make(allocPerm, initPermSize, storePerms);
}

Stmt SqueezedModeFormat::getInitYieldPos(Expr prevSize, Mode mode) const {
  Expr rpermArr = getRperm(mode);
  Stmt declRperm = VarDecl::make(rpermArr, 0);
  Stmt allocRperm = Allocate::make(rpermArr, getSizeArray(mode.getModePack()));
  Expr permSize = getLocalPermSize(mode);
  Expr permSizeArray = getPermSizeArray(mode.getModePack());
  Stmt copyPermSize = VarDecl::make(permSize, Load::make(permSizeArray, 0));
  Expr pVar = Var::make("p", Int());
  Expr permArray = getPermArray(mode.getModePack());
  Stmt initRperm = For::make(pVar, 0, permSize, 1, 
                             Store::make(rpermArr, Load::make(permArray, pVar), pVar));
  return Block::make(declRperm, allocRperm, copyPermSize, initRperm);
}

ModeFunction SqueezedModeFormat::getYieldPos(Expr parentPos, 
    std::vector<Expr> coords, Mode mode) const {
  const auto pos = ir::Add::make(ir::Mul::make(parentPos, getLocalPermSize(mode)),
                                 Load::make(getRperm(mode), coords.back()));
  return ModeFunction(Stmt(), {pos});
}

Stmt SqueezedModeFormat::getFinalizeLevel(Mode mode) const {
  return Free::make(getRperm(mode));
}

std::vector<Expr> SqueezedModeFormat::getArrays(Expr tensor, int mode, 
                                                int level) const {
  const auto arraysPrefix = util::toString(tensor) + std::to_string(level);
  return {GetProperty::make(tensor, TensorProperty::Indices,
                            level - 1, 0, arraysPrefix + "_perm"),
          GetProperty::make(tensor, TensorProperty::Indices,
                            level - 1, 1, arraysPrefix + "_nzslice"),
          GetProperty::make(tensor, TensorProperty::Dimension, mode)};
}

Expr SqueezedModeFormat::getPermArray(ModePack pack) const {
  return pack.getArray(0);
}

Expr SqueezedModeFormat::getPermSizeArray(ModePack pack) const {
  return pack.getArray(1);
}

Expr SqueezedModeFormat::getSizeArray(ModePack pack) const {
  return pack.getArray(2);
}

Expr SqueezedModeFormat::getRperm(Mode mode) const {
  const std::string varName = mode.getName() + "_rperm";
  
  if (!mode.hasVar(varName)) {
    Expr ptr = Var::make(varName, Int(), true);
    mode.addVar(varName, ptr);
    return ptr;
  }

  return mode.getVar(varName);
}

Expr SqueezedModeFormat::getLocalPermSize(Mode mode) const {
  const std::string varName = mode.getName() + "_nzslice_local";
  
  if (!mode.hasVar(varName)) {
    Expr ptr = Var::make(varName, Int());
    mode.addVar(varName, ptr);
    return ptr;
  }

  return mode.getVar(varName);
}

}
