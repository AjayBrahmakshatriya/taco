#include "taco/lower/lower.h"

#include <algorithm>
#include <vector>
#include <stack>
#include <set>

#include "taco/tensor.h"
#include "taco/expr.h"

#include "taco/ir/ir.h"
#include "taco/ir/ir_visitor.h"
#include "ir/ir_codegen.h"

#include "lower_codegen.h"
#include "iterators.h"
#include "tensor_path.h"
#include "merge_lattice.h"
#include "iteration_schedule.h"
#include "expr_tools.h"
#include "taco/expr_nodes/expr_nodes.h"
#include "taco/expr_nodes/expr_rewriter.h"
#include "storage/iterator.h"
#include "taco/util/name_generator.h"
#include "taco/util/collections.h"
#include "taco/util/strings.h"

using namespace std;

namespace taco {
namespace lower {

using namespace taco::ir;
using namespace taco::expr_nodes;
using taco::storage::Iterator;

struct Context {
  /// Determines what kind of code to emit (e.g. compute and/or assembly)
  set<Property>        properties;

  /// The iteration schedule to use for lowering the index expression
  IterationSchedule    schedule;

  /// The iterators of the tensor tree levels
  Iterators            iterators;

  /// The size of initial memory allocations
  Expr                 allocSize;

  /// Variable representing capacity of values array
  Expr                 valsCapacity;

  /// The size at which the values array should be incremented
  Expr                 valsInc;

  /// The level at which the values array should be incremented
  IndexVar             incLevel;

  /// Maps tensor (scalar) temporaries to IR variables.
  /// (Not clear if this approach to temporaries is too hacky.)
  map<TensorBase,Expr> temporaries;
};

struct Target {
  Expr tensor;
  Expr pos;
};

enum ComputeCase {
  // Emit the last free variable. We first recurse to compute remaining
  // reduction variables into a temporary, before we compute and store the
  // main expression
  LAST_FREE,

  // Emit a variable above the last free variable. First emit code to compute
  // available expressions and store them in temporaries, before
  // we recurse on the next index variable.
  ABOVE_LAST_FREE,

  // Emit a variable below the last free variable. First recurse to emit
  // remaining (summation) variables, before we add in the available expressions
  // for the current summation variable.
  BELOW_LAST_FREE
};

static ComputeCase getComputeCase(const IndexVar& indexVar,
                                  const IterationSchedule& schedule) {
  if (schedule.isLastFreeVariable(indexVar)) {
    return LAST_FREE;
  }
  else if (schedule.hasFreeVariableDescendant(indexVar)) {
    return ABOVE_LAST_FREE;
  }
  else {
    return BELOW_LAST_FREE;
  }
}

/// Returns true iff the lattice must be merged, false otherwise. A lattice
/// must be merged iff it has more than one lattice point, or two or more of
/// its iterators are not random access.
static bool needsMerge(MergeLattice lattice) {
  if (lattice.getSize() > 1) {
    return true;
  }

  int notRandomAccess = 0;
  for (auto& iterator : lattice.getIterators()) {
    if ((!iterator.isRandomAccess()) && (++notRandomAccess > 1)) {
      return true;
    }
  }
  return false;
}

static bool needsZero(const Context& ctx, 
                      const std::vector<IndexVar>& resultIdxVars) {
  const auto& resultTensorPath = ctx.schedule.getResultTensorPath();
  for (const auto& idxVar : resultIdxVars) {
    if (ctx.iterators[resultTensorPath.getStep(idxVar)].isRandomAccess()) {
      for (const auto& tensorPath : ctx.schedule.getTensorPaths()) {
        if (util::contains(tensorPath.getVariables(), idxVar) && 
            !ctx.iterators[tensorPath.getStep(idxVar)].isDense()) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool needsZero(const Context& ctx) {
  const auto& schedule = ctx.schedule;
  const auto& resultIdxVars = schedule.getResultTensorPath().getVariables();

  if (schedule.hasReductionVariableAncestor(resultIdxVars.back())) {
    return true;
  }

  return needsZero(ctx, resultIdxVars);
}

static IndexExpr emitAvailableExprs(const IndexVar& indexVar,
                                    const IndexExpr& indexExpr, Context& ctx,
                                    vector<Stmt>& stmts) {
  vector<IndexVar>  visited    = ctx.schedule.getAncestors(indexVar);
  vector<IndexExpr> availExprs = getAvailableExpressions(indexExpr, visited);
  map<IndexExpr,IndexExpr> substitutions;
  for (const IndexExpr& availExpr : availExprs) {
    TensorBase t("t" + indexVar.getName(), Float(64));
    substitutions.insert({availExpr, taco::Access(t)});
    Expr tensorVar = Var::make(t.getName(), Float(64));
    ctx.temporaries.insert({t, tensorVar});
    Expr expr = lowerToScalarExpression(availExpr, ctx.iterators,
                                        ctx.schedule, ctx.temporaries, stmts);
    stmts.push_back(VarAssign::make(tensorVar, expr, true));
  }
  return replace(indexExpr, substitutions);
}

static void emitComputeExpr(const Target& target, const IndexVar& indexVar,
                            const IndexExpr& indexExpr, const Context& ctx,
                            vector<Stmt>& stmts) {
  Expr expr = lowerToScalarExpression(indexExpr, ctx.iterators,
                                      ctx.schedule, ctx.temporaries, stmts);
  if (target.pos.defined()) {
    Stmt store = ctx.schedule.hasReductionVariableAncestor(indexVar)
        ? compoundStore(target.tensor, target.pos, expr)
        :   Store::make(target.tensor, target.pos, expr);
    stmts.push_back(store);
  }
  else {
    Stmt assign = ctx.schedule.hasReductionVariableAncestor(indexVar)
        ?  compoundAssign(target.tensor, expr)
        : VarAssign::make(target.tensor, expr);
    stmts.push_back(assign);
  }
}

static LoopKind doParallelize(const IndexVar& indexVar, const Expr& tensor, 
                              const Context& ctx) {
  if (ctx.schedule.getAncestors(indexVar).size() != 1 ||
      ctx.schedule.isReduction(indexVar)) {
    return LoopKind::Serial;
  }

  const TensorPath& resultPath = ctx.schedule.getResultTensorPath();
  for (size_t i = 0; i < resultPath.getSize(); i++){
    if (!ctx.iterators[resultPath.getStep(i)].isDense()) {
      return LoopKind::Serial;
    }
  }

  const TensorPath parallelizedAccess = [&]() {
    const auto tensorName = tensor.as<Var>()->name;
    for (const auto& tensorPath : ctx.schedule.getTensorPaths()) {
      if (tensorPath.getTensor().getName() == tensorName) {
        return tensorPath;
      }
    }
    taco_iassert(false);
    return TensorPath();
  }();

  if (parallelizedAccess.getSize() > 2) {
    for (size_t i = 1; i < parallelizedAccess.getSize(); ++i) {
      if (!ctx.iterators[parallelizedAccess.getStep(i)].isDense()) {
        return LoopKind::Dynamic;
      }
    }
  }

  return LoopKind::Static;
}

/// Expression evaluates to true iff none of the iteratators are exhausted
static Expr noneExhausted(const vector<Iterator>& iterators) {
  vector<Expr> stepIterLqEnd;
  for (auto& iter : iterators) {
    stepIterLqEnd.push_back(Lt::make(iter.getIteratorVar(), iter.end()));
  }
  return conjunction(stepIterLqEnd);
}

/// Expression evaluates to true iff all the iterator idx vars are equal to idx
/// or if there are no iterators.
static Expr allEqualTo(const vector<Iterator>& iterators, Expr idx) {
  if (iterators.size() == 0) {
    return Literal::make(true);
  }

  vector<Expr> iterIdxEqualToIdx;
  for (auto& iter : iterators) {
    iterIdxEqualToIdx.push_back(Eq::make(iter.getIdxVar(), idx));
  }
  return conjunction(iterIdxEqualToIdx);
}

/// Returns the iterator for the `idx` variable from `iterators`, or Iterator()
/// none of the iterator iterate over `idx`.
static Iterator getIterator(const Expr& idx,
                            const vector<Iterator>& iterators) {
  for (auto& iterator : iterators) {
    if (iterator.getIdxVar() == idx) {
      return iterator;
    }
  }
  return Iterator();
}

static vector<Iterator> removeIterator(const Expr& idx,
                                       const vector<Iterator>& iterators) {
  vector<Iterator> result;
  for (auto& iterator : iterators) {
    if (iterator.getIdxVar() != idx) {
      result.push_back(iterator);
    }
  }
  return result;
}

static Stmt createIfStatements(vector<pair<Expr,Stmt>> cases,
                               const MergeLattice& lattice) {
  if (!needsMerge(lattice)) {
    return cases[0].second;
  }

  vector<pair<Expr,Stmt>> ifCases;
  pair<Expr,Stmt> elseCase;
  for (auto& cas : cases) {
    auto lit = cas.first.as<Literal>();
    if (lit != nullptr && lit->type == Bool() && lit->value == 1){
      taco_iassert(!elseCase.first.defined()) <<
          "there should only be one true case";
      elseCase = cas;
    }
    else {
      ifCases.push_back(cas);
    }
  }

  if (elseCase.first.defined()) {
    ifCases.push_back(elseCase);
    return Case::make(ifCases, true);
  }
  else {
    return Case::make(ifCases, lattice.isFull());
  }
}

static vector<Stmt> lower(const Target&    target,
                          const IndexExpr& indexExpr,
                          const IndexVar&  indexVar,
                          Context&         ctx) {
  vector<Stmt> code;

  MergeLattice lattice = MergeLattice::make(indexExpr, indexVar, ctx.schedule,
                                            ctx.iterators);

  TensorPath     resultPath     = ctx.schedule.getResultTensorPath();
  TensorPathStep resultStep     = resultPath.getStep(indexVar);
  Iterator       resultIterator = (resultStep.getPath().defined())
                                  ? ctx.iterators[resultStep]
                                  : Iterator();

  const bool emitCompute  = util::contains(ctx.properties, Compute);
  const bool emitAssemble = util::contains(ctx.properties, Assemble);
  const bool emitMerge    = needsMerge(lattice);

  // Emit compute code for three cases: above, at or below the last free var
  const ComputeCase indexVarCase = getComputeCase(indexVar, ctx.schedule);

  // Emit code to initialize pos variables:
  // B2_pos = B2_pos_arr[B1_pos];
  for (auto& iterator : lattice.getIterators()) {
    Expr iteratorVar = iterator.getIteratorVar();
    Stmt iteratorInit = VarAssign::make(iteratorVar, iterator.begin(), true);
    code.push_back(iteratorInit);
  }

  // Emit one loop per lattice point lp
  vector<Stmt> loops;
  for (MergeLatticePoint lp : lattice) {
    MergeLattice lpLattice = lattice.getSubLattice(lp);
    auto lpIterators = lp.getIterators();

    vector<Stmt> loopBody;

    // Emit code to initialize sequential access idx variables:
    // int kB = B1_idx_arr[B1_pos];
    // int kc = c0_idx_arr[c0_pos];
    vector<Expr> mergeIdxVariables;
    auto sequentialAccessIterators = getSequentialAccessIterators(lpIterators);
    for (Iterator& iterator : sequentialAccessIterators) {
      Stmt initIdx = iterator.initDerivedVar();
      loopBody.push_back(initIdx);
      mergeIdxVariables.push_back(iterator.getIdxVar());
    }

    // Emit code to initialize the index variable:
    // k = min(kB, kc);
    Expr idx = (lp.getMergeIterators().size() > 1)
               ? min(indexVar.getName(), lp.getMergeIterators(), &loopBody)
               : lp.getMergeIterators()[0].getIdxVar();

    // Emit code to initialize random access pos variables:
    // D1_pos = (D0_pos * 3) + k;
    auto randomAccessIterators =
        getRandomAccessIterators(util::combine(lpIterators, {resultIterator}));
    for (Iterator& iterator : randomAccessIterators) {
      Expr val = Add::make(Mul::make(iterator.getParent().getPtrVar(),
                                     iterator.end()), idx);
      Stmt initPos = VarAssign::make(iterator.getPtrVar(), val, true);
      loopBody.push_back(initPos);
    }

    // Emit code to initialize (pos_)end variables:
    for (Iterator& iterator : lpIterators) {
      Expr val = Add::make(iterator.getPtrVar(), 1);
      Stmt initEnd = VarAssign::make(iterator.getEndVar(), val, true);
      loopBody.push_back(initEnd);

      if (iterator.hasDuplicates()) {
        Expr tensorIdx = iterator.getIdxVar();
        Expr endVar = iterator.getEndVar();

        Expr reachedEnd = Lt::make(endVar, iterator.end());
        Expr isDuplicate = Eq::make(iterator.getIdx(endVar), idx);
        Expr doAdvance = And::make(reachedEnd, isDuplicate);
        Stmt incEnd = VarAssign::make(endVar, Add::make(endVar, 1));
        Stmt ffLoop = While::make(doAdvance, incEnd);
        if (tensorIdx != idx) {
          ffLoop = IfThenElse::make(Eq::make(tensorIdx, idx), ffLoop);
        }

        loopBody.push_back(ffLoop);
      }
    }

    Expr resultStartVar;
    if (resultIterator.defined() && resultIterator.isSequentialAccess() && 
        indexVarCase == ABOVE_LAST_FREE) {
      auto resultNextStep = resultPath.getStep(resultStep.getStep() + 1);
      auto resultNextIterator = ctx.iterators[resultNextStep];

      if (resultNextIterator.isSequentialAccess()) {
        Expr nextIteratorVar = resultNextIterator.getIteratorVar();
        const std::string nextIteratorName = nextIteratorVar.as<Var>()->name;
        resultStartVar = Var::make(nextIteratorName + "_start", Type(Type::Int));

        Expr resultNextPos = resultNextIterator.getPtrVar();
        Stmt initStartVar = VarAssign::make(resultStartVar, resultNextPos, true);
        loopBody.push_back(initStartVar);
      }
    }

    if (emitAssemble && emitCompute && indexVar == ctx.incLevel) {
      Expr vals = GetProperty::make(resultIterator.getTensor(),
                                    TensorProperty::Values);
      Expr resultPos = resultIterator.getPtrVar();

      std::vector<Stmt> body;

      Expr newValsEnd = Mul::make(Add::make(resultPos, 1), ctx.valsInc);
      Expr newCapacity = Mul::make(2, newValsEnd);

      const auto name = ctx.valsCapacity.as<Var>()->name + "_new";
      Expr newCapacityVar = Var::make(name, Type(Type::Int));
      Stmt initNewCapacity = VarAssign::make(newCapacityVar, newCapacity, true);
      body.push_back(initNewCapacity);

      Stmt resizeVals = Allocate::make(vals, newCapacityVar, true);
      body.push_back(resizeVals);

      if (!isa<Literal>(ctx.valsInc)) {
        const auto& resultIdxVars = resultPath.getVariables();
        const auto it = std::find(resultIdxVars.begin(), 
                                  resultIdxVars.end(), indexVar);
        std::vector<IndexVar> nextIdxVars(it, resultIdxVars.end());

        if (needsZero(ctx, nextIdxVars)) {
          auto idxVarName = resultIterator.getTensor().as<Var>()->name + "_pos";
          Expr idxVar = Var::make(idxVarName, Type(Type::Int));
          Stmt zeroStmt = Store::make(target.tensor, idxVar, 0.0);
          Stmt zeroLoop = For::make(idxVar, ctx.valsCapacity, 
                                    newCapacityVar, 1, zeroStmt);
          body.push_back(zeroLoop);
        }
      }
      
      Stmt updateCapacity = VarAssign::make(ctx.valsCapacity, newCapacityVar);
      body.push_back(updateCapacity);

      Expr shouldResize = Lte::make(ctx.valsCapacity, newValsEnd);
      Stmt maybeResizeVals = IfThenElse::make(shouldResize, Block::make(body));
      loopBody.push_back(maybeResizeVals);
    }

    // Emit one case per lattice point in the sub-lattice rooted at lp
    vector<pair<Expr,Stmt>> cases;
    for (MergeLatticePoint& lq : lpLattice) {
      IndexExpr lqExpr = lq.getExpr();
      vector<Stmt> caseBody;

      // Emit available sub-expressions at this loop level
      if (emitCompute && ABOVE_LAST_FREE == indexVarCase) {
        lqExpr = emitAvailableExprs(indexVar, lqExpr, ctx, caseBody);
      }

      // Recursive call to emit iteration schedule children
      for (auto& child : ctx.schedule.getChildren(indexVar)) {
        IndexExpr childExpr = lqExpr;
        Target childTarget = target;
        if (indexVarCase == LAST_FREE || indexVarCase == BELOW_LAST_FREE) {
          // Extract the expression to compute at the next level. If there's no
          // computation on the next level for this lattice case then skip it
          childExpr = getSubExpr(lqExpr, ctx.schedule.getDescendants(child));
          if (!childExpr.defined()) continue;

          // Reduce child expression into temporary
          TensorBase t("t" + child.getName(), Float(64));
          Expr tensorVar = Var::make(t.getName(), Type(Type::Float,64));
          ctx.temporaries.insert({t, tensorVar});
          childTarget.tensor = tensorVar;
          childTarget.pos    = Expr();
          if (emitCompute) {
            caseBody.push_back(VarAssign::make(tensorVar, 0.0, true));
          }

          // Rewrite lqExpr to substitute the expression computed at the next
          // level with the temporary
          lqExpr = replace(lqExpr, {{childExpr,taco::Access(t)}});
        }
        auto childCode = lower::lower(childTarget, childExpr, child, ctx);
        util::append(caseBody, childCode);
      }

      // Emit code to compute and store/assign result 
      if (emitCompute &&
          (indexVarCase == LAST_FREE || indexVarCase == BELOW_LAST_FREE)) {
        emitComputeExpr(target, indexVar, lqExpr, ctx, caseBody);
      }

      // Emit code to increment the result `pos` variable and to allocate
      // additional storage for result `idx` and `pos` arrays
      if (resultIterator.defined() && resultIterator.isSequentialAccess()) {
        Expr rpos = resultIterator.getPtrVar();
        Stmt posInc = VarAssign::make(rpos, Add::make(rpos, 1));

        if (emitAssemble){
          // Emit a store of the index variable value to the result `idx` array
          // A2_idx_arr[A2_pos] = j;
          Stmt idxStore = resultIterator.storeIdx(idx);
          if (idxStore.defined()) {
            posInc = Block::make({idxStore, posInc});
          }
        }

        // Only increment `pos` if values were produced at the next level
        if (indexVarCase == ABOVE_LAST_FREE) {
          auto resultNextStep = resultPath.getStep(resultStep.getStep() + 1);
          auto resultNextIterator = ctx.iterators[resultNextStep];

          if (emitAssemble || resultNextIterator.isRandomAccess()) {
            auto nextIteratorVar = resultNextIterator.getIteratorVar();
            auto nextIteratorName = nextIteratorVar.as<Var>()->name;

            const std::string insertedName = nextIteratorName + "_inserted";
            Expr resultInsertedVar = Var::make(insertedName, Type(Type::Int));

            Expr nextIteratorPos = resultNextIterator.getPtrVar();
            Expr inserted = resultStartVar.defined() ? 
                            Sub::make(nextIteratorPos, resultStartVar) : 
                            resultNextIterator.getRangeSize();
            taco_iassert(inserted.defined() && 
                         (!isa<Literal>(inserted) || inserted.as<Literal>() > 0));

            Stmt initResultInserted = VarAssign::make(resultInsertedVar, 
                                                      inserted, true);
            caseBody.push_back(initResultInserted);

            if (resultIterator.hasDuplicates() && 
                resultNextIterator.isFixedRange() && 
                !resultNextIterator.isRandomAccess()) {
              Expr itVar = Var::make("it", Type(Type::Int));
              posInc = For::make(itVar, 0, resultInsertedVar, 
                                 resultNextIterator.getRangeSize(), posInc);
            }

            if (resultStartVar.defined()) { 
              posInc = IfThenElse::make(Gt::make(resultInsertedVar, 0), posInc);
            }
            caseBody.push_back(posInc);
          }
        } else {
          caseBody.push_back(posInc);
        }
      }

      auto caseIterators = removeIterator(idx, lq.getRangeIterators());
      cases.push_back({allEqualTo(caseIterators,idx), Block::make(caseBody)});
    }
    loopBody.push_back(createIfStatements(cases, lpLattice));

    Stmt loop;
    // Emit loop (while loop for merges and for loop for non-merges)
    if (emitMerge || lp.getRangeIterators().front().hasDuplicates()) {
      // Emit code to increment sequential access `pos` variables. Variables 
      // that may not be consumed in an iteration (i.e. their iteration space is
      // different from the loop iteration space) are guarded by a conditional:

      // if (k == kB) B1_pos++;
      // if (k == kc) c0_pos++;
      for (auto& iterator : removeIterator(idx, lp.getRangeIterators())) {
        Expr ivar = iterator.getIteratorVar();
        Stmt inc = VarAssign::make(ivar, iterator.getEndVar());
        Expr tensorIdx = iterator.getIdxVar();
        loopBody.push_back(IfThenElse::make(Eq::make(tensorIdx, idx), inc));
      }

      /// k++
      auto idxIterator = getIterator(idx, lpIterators);
      if (idxIterator.defined()) {
        Expr ivar = idxIterator.getIteratorVar();
        loopBody.push_back(VarAssign::make(ivar, idxIterator.getEndVar()));
      }
      
      loop = While::make(noneExhausted(lp.getRangeIterators()),
                         Block::make(loopBody));
    } else {
      Iterator iter = lp.getRangeIterators().front();
      loop = For::make(iter.getIteratorVar(), iter.begin(), iter.end(), 1,
                       Block::make(loopBody), 
                       doParallelize(indexVar, iter.getTensor(), ctx));
      
      // TODO: pos variable is initialized in the for loop, so don't need to 
      // initialize it beforehand
      // code.clear();
    }
    loops.push_back(loop);
  }
  util::append(code, loops);

  // Emit a store of the  segment size to the result pos index
  // A2_pos_arr[A1_pos + 1] = A2_pos;
  if (emitAssemble && resultIterator.defined()) {
    Stmt posStore = resultIterator.storePtr(resultIterator.getPtrVar(), Expr());
    if (posStore.defined()) {
      util::append(code, {posStore});
    }
  }

  return code;
}

Stmt lower(TensorBase tensor, string funcName, set<Property> properties) {
  Context ctx;
  ctx.allocSize  = Var::make("init_alloc_size", Type(Type::Int));
  ctx.properties = properties;

  const bool emitAssemble = util::contains(ctx.properties, Assemble);
  const bool emitCompute = util::contains(ctx.properties, Compute);

  auto name = tensor.getName();
  auto indexExpr = tensor.getExpr();

  // Pack the tensor and it's expression operands into the parameter list
  vector<Expr> parameters;
  vector<Expr> results;
  map<TensorBase,Expr> tensorVars;
  tie(parameters,results,tensorVars) = getTensorVars(tensor);

  // Create the schedule and the iterators of the lowered code
  ctx.schedule = IterationSchedule::make(tensor);
  ctx.iterators = Iterators(ctx.schedule, tensorVars);

  vector<Stmt> init, body;

  TensorPath resultPath = ctx.schedule.getResultTensorPath();
  if (emitAssemble) {
    for (auto& indexVar : resultPath.getVariables()) {
      Iterator iter = ctx.iterators[resultPath.getStep(indexVar)];
      Stmt allocStmts = iter.initStorage(ctx.allocSize);
      if (allocStmts.defined()) {
        if (init.empty()) {
          Expr allocSize = (int)tensor.getAllocSize();
          Stmt setAllocSize = VarAssign::make(ctx.allocSize, allocSize, true);
          init.push_back(setAllocSize);
        }
        init.push_back(allocStmts);
      }
    }
  }

  // Initialize the result pos variables
  if (emitCompute || emitAssemble) {
    Stmt prevIteratorInit;
    for (auto& indexVar : resultPath.getVariables()) {
      Iterator iter = ctx.iterators[resultPath.getStep(indexVar)];
      if (iter.isSequentialAccess()) {
        // Emit code to initialize the result pos variable
        Stmt iteratorInit = VarAssign::make(iter.getPtrVar(), 0, true);
        body.push_back(iteratorInit);
      }
    }
  }
  taco_iassert(results.size() == 1) << "An expression can only have one result";

  // Lower the iteration schedule
  auto& roots = ctx.schedule.getRoots();

  // Lower tensor expressions
  if (roots.size() > 0) {
    Iterator resultIterator = (resultPath.getSize() > 0)
        ? ctx.iterators[resultPath.getLastStep()]
        : ctx.iterators.getRoot(resultPath);  // e.g. `a = b(i) * c(i)`
    Target target;
    target.tensor = GetProperty::make(resultIterator.getTensor(),
                                      TensorProperty::Values);
    target.pos = resultIterator.getPtrVar();

    if (emitCompute) {
      Expr size = 1;
      for (auto& indexVar : resultPath.getVariables()) {
        const Iterator iter = ctx.iterators[resultPath.getStep(indexVar)];
        if (!iter.isFixedRange()) {
          size = ctx.allocSize;
          break;
        }
        size = Mul::make(size, iter.end());
      }

      if (emitAssemble) {
        auto valsCapacityName = name + "_vals_capacity";
        ctx.valsCapacity = Var::make(valsCapacityName, Type(Type::Int));

        Stmt initValsCapacity = VarAssign::make(ctx.valsCapacity, size, true);
        Stmt allocVals = Allocate::make(target.tensor, ctx.valsCapacity);

        init.push_back(initValsCapacity);
        init.push_back(allocVals);
      }

      if (size != ctx.allocSize) {
        // If the output is dense and if either an output dimension is merged 
        // with a sparse input dimension or if the emitted code is a scatter 
        // code, then we also need to zero the output.
        if (isa<Literal>(size)) {
          taco_iassert(to<Literal>(size)->value == 1);
          body.push_back(Store::make(target.tensor, 0, 0.0));
        } else if (needsZero(ctx)) {
          Expr idxVar = Var::make(name + "_pos", Type(Type::Int));
          Stmt zeroStmt = Store::make(target.tensor, idxVar, 0.0);
          body.push_back(For::make(idxVar, 0, size, 1, zeroStmt));
        }
      } else if (emitAssemble) {
        ctx.valsInc = 1;
        for (auto& indexVar : resultPath.getVariables()) {
          Iterator iter = ctx.iterators[resultPath.getStep(indexVar)];
          if (iter.isFixedRange()) {
            ctx.valsInc = Mul::make(ctx.valsInc, iter.getRangeSize());
          } else {
            ctx.valsInc = 1;
            ctx.incLevel = indexVar;
          }
        }
        
        if (!isa<Literal>(ctx.valsInc)) {
          const auto& resultIdxVars = resultPath.getVariables();
          const auto it = std::find(resultIdxVars.begin(), 
                                    resultIdxVars.end(), ctx.incLevel);
          std::vector<IndexVar> nextIdxVars(it, resultIdxVars.end());

          if (needsZero(ctx, nextIdxVars)) {
            Expr idxVar = Var::make(name + "_pos", Type(Type::Int));
            Stmt zeroStmt = Store::make(target.tensor, idxVar, 0.0);
            Stmt zeroLoop = For::make(idxVar, 0, ctx.valsCapacity, 1, zeroStmt);
            init.push_back(zeroLoop);
          }
        }
      }
    }

    const bool emitLoops = emitCompute || (emitAssemble && [&]() {
      for (auto& indexVar : resultPath.getVariables()) {
        Iterator iter = ctx.iterators[resultPath.getStep(indexVar)];
        if (!iter.isDense()) {
          return true;
        }
      }
      return false;
    }());
    if (emitLoops) {
      for (auto& root : roots) {
        auto loopNest = lower::lower(target, indexExpr, root, ctx);
        util::append(body, loopNest);
      }
    }

    if (emitAssemble && !emitCompute) {
      Expr size = 1;
      for (auto& indexVar : resultPath.getVariables()) {
        Iterator iter = ctx.iterators[resultPath.getStep(indexVar)];
        size = iter.isFixedRange() ? Mul::make(size, iter.end()) : 
               iter.getPtrVar();
      }
      Stmt allocVals = Allocate::make(target.tensor, size);
      
      if (!body.empty()) {
        body.push_back(BlankLine::make());
      }
      body.push_back(allocVals);
    }
  }
  // Lower scalar expressions
  else {
    TensorPath resultPath = ctx.schedule.getResultTensorPath();
    Expr resultTensorVar = ctx.iterators.getRoot(resultPath).getTensor();
    Expr vals = GetProperty::make(resultTensorVar, TensorProperty::Values);
    if (emitAssemble) {
      Stmt allocVals = Allocate::make(vals, 1);
      init.push_back(allocVals);
    }
    if (emitCompute) {
      Expr expr = lowerToScalarExpression(indexExpr, ctx.iterators, ctx.schedule,
                                          map<TensorBase,Expr>(), body);
      Stmt compute = Store::make(vals, 0, expr);
      body.push_back(compute);
    }
  }

  if (!init.empty()) {
    init.push_back(BlankLine::make());
  }
  body = util::combine(init, body);

  return Function::make(funcName, parameters, results, Block::make(body));
}
}}
