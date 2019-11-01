#include "taco/index_notation/index_notation_rewriter.h"

#include "taco/index_notation/index_notation_nodes.h"
#include "taco/util/collections.h"

#include <vector>

using namespace std;

namespace taco {

// class IndexVarExprRewriterStrict
IndexVarExpr IndexVarExprRewriterStrict::rewrite(IndexVarExpr e) {
  if (e.defined()) {
    e.accept(this);
    e = expr;
  }
  else {
    e = IndexVarExpr();
  }
  expr = IndexVarExpr();
  return e;
}


// class IndexExprRewriterStrict
IndexExpr IndexExprRewriterStrict::rewrite(IndexExpr e) {
  if (e.defined()) {
    e.accept(this);
    e = expr;
  }
  else {
    e = IndexExpr();
  }
  expr = IndexExpr();
  return e;
}


// class IndexStmtRewriterStrict
IndexStmt IndexStmtRewriterStrict::rewrite(IndexStmt s) {
  if (s.defined()) {
    s.accept(this);
    s = stmt;
  }
  else {
    s = IndexStmt();
  }
  stmt = IndexStmt();
  return s;
}


// class IndexVarExprRewriter
void IndexVarExprRewriter::visit(const IndexVarAccessNode* op) {
  expr = op;
}

void IndexVarExprRewriter::visit(const IndexVarLiteralNode* op) {
  expr = op;
}

template <class T>
IndexVarExpr visitBinaryOp(const T *op, IndexVarExprRewriter *rw) {
  IndexVarExpr a = rw->rewrite(op->a);
  IndexVarExpr b = rw->rewrite(op->b);
  if (a == op->a && b == op->b) {
    return op;
  }
  else {
    return new T(a, b);
  }
}

void IndexVarExprRewriter::visit(const IndexVarAddNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexVarExprRewriter::visit(const IndexVarSubNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexVarExprRewriter::visit(const IndexVarDivNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexVarExprRewriter::visit(const IndexVarCountNode* op) {
  expr = op;
}


// class IndexExprRewriter
void IndexNotationRewriter::visit(const AccessNode* op) {
  expr = op;
}

void IndexNotationRewriter::visit(const SlicedAccessNode* op) {
  expr = op;
}

template <class T>
IndexExpr visitUnaryOp(const T *op, IndexNotationRewriter *rw) {
  IndexExpr a = rw->rewrite(op->a);
  if (a == op->a) {
    return op;
  }
  else {
    return new T(a);
  }
}

void IndexNotationRewriter::visit(const LiteralNode* op) {
  expr = op;
}

template <class T>
IndexExpr visitBinaryOp(const T *op, IndexNotationRewriter *rw) {
  IndexExpr a = rw->rewrite(op->a);
  IndexExpr b = rw->rewrite(op->b);
  if (a == op->a && b == op->b) {
    return op;
  }
  else {
    return new T(a, b);
  }
}

void IndexNotationRewriter::visit(const NegNode* op) {
  expr = visitUnaryOp(op, this);
}

void IndexNotationRewriter::visit(const SqrtNode* op) {
  expr = visitUnaryOp(op, this);
}

void IndexNotationRewriter::visit(const AddNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexNotationRewriter::visit(const SubNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexNotationRewriter::visit(const MulNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexNotationRewriter::visit(const DivNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexNotationRewriter::visit(const MaxNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexNotationRewriter::visit(const MinNode* op) {
  expr = visitBinaryOp(op, this);
}

void IndexNotationRewriter::visit(const CastNode* op) {
  IndexExpr a = rewrite(op->a);
  if (a == op->a) {
    expr = op;
  }
  else {
    expr = new CastNode(a, op->getDataType());
  }
}

void IndexNotationRewriter::visit(const MapNode* op) {
  IndexExpr in = rewrite(op->in);
  IndexExpr out = rewrite(op->out);
  if (in == op->in && out == op->out) {
    expr = op;
  }
  else {
    expr = new MapNode(in, out);
  }
}

void IndexNotationRewriter::visit(const CallIntrinsicNode* op) {
  std::vector<IndexExpr> args;
  bool rewritten = false;
  for (auto& arg : op->args) {
    IndexExpr rewrittenArg = rewrite(arg);
    args.push_back(rewrittenArg);
    if (arg != rewrittenArg) {
      rewritten = true;
    }
  }
  if (rewritten) {
    expr = new CallIntrinsicNode(op->func, args);
  }
  else {
    expr = op;
  }
}

void IndexNotationRewriter::visit(const ReductionNode* op) {
  IndexExpr a = rewrite(op->a);
  if (a == op->a) {
    expr = op;
  }
  else {
    expr = new ReductionNode(op->op, op->var, a);
  }
}

void IndexNotationRewriter::visit(const AssignmentNode* op) {
  // A design decission is to not visit the rhs access expressions or the op,
  // as these are considered part of the assignment.  When visiting access
  // expressions, therefore, we only visit read access expressions.
  IndexExpr rhs = rewrite(op->rhs);
  if (rhs == op->rhs) {
    stmt = op;
  }
  else {
    stmt = new AssignmentNode(op->lhs, rhs, op->op);
  }
}

void IndexNotationRewriter::visit(const YieldNode* op) {
  IndexExpr expr = rewrite(op->expr);
  if (expr == op->expr) {
    stmt = op;
  } else {
    stmt = new YieldNode(op->indexVars, expr);
  }
}

void IndexNotationRewriter::visit(const ForallNode* op) {
  IndexStmt s = rewrite(op->stmt);
  if (s == op->stmt) {
    stmt = op;
  }
  else {
    stmt = new ForallNode(op->indexVar, s, op->tags);
  }
}

void IndexNotationRewriter::visit(const WhereNode* op) {
  IndexStmt producer = rewrite(op->producer);
  IndexStmt consumer = rewrite(op->consumer);
  if (producer == op->producer && consumer == op->consumer) {
    stmt = op;
  }
  else {
    stmt = new WhereNode(consumer, producer);
  }
}

void IndexNotationRewriter::visit(const SequenceNode* op) {
  IndexStmt definition = rewrite(op->definition);
  IndexStmt mutation = rewrite(op->mutation);
  if (definition == op->definition && mutation == op->mutation) {
    stmt = op;
  }
  else {
    stmt = new SequenceNode(definition, mutation);
  }
}

void IndexNotationRewriter::visit(const MultiNode* op) {
  IndexStmt stmt1 = rewrite(op->stmt1);
  IndexStmt stmt2 = rewrite(op->stmt2);
  if (stmt1 == op->stmt1 && stmt2 == op->stmt2) {
    stmt = op;
  }
  else {
    stmt = new MultiNode(stmt1, stmt2);
  }
}


// Functions
#define SUBSTITUTE_EXPR                        \
do {                                           \
  IndexExpr e = op;                            \
  if (util::contains(exprSubstitutions, e)) {  \
    expr = exprSubstitutions.at(e);            \
  }                                            \
  else {                                       \
    IndexNotationRewriter::visit(op);          \
  }                                            \
} while(false)

#define SUBSTITUTE_STMT                        \
do {                                           \
  IndexStmt s = op;                            \
  if (util::contains(stmtSubstitutions, s)) {  \
    stmt = stmtSubstitutions.at(s);            \
  }                                            \
  else {                                       \
    IndexNotationRewriter::visit(op);          \
  }                                            \
} while(false)

struct ReplaceRewriter : public IndexNotationRewriter {
  const std::map<IndexExpr,IndexExpr>& exprSubstitutions;
  const std::map<IndexStmt,IndexStmt>& stmtSubstitutions;

  ReplaceRewriter(const std::map<IndexExpr,IndexExpr>& exprSubstitutions,
                  const std::map<IndexStmt,IndexStmt>& stmtSubstitutions)
      : exprSubstitutions(exprSubstitutions),
        stmtSubstitutions(stmtSubstitutions) {}

  using IndexNotationRewriter::visit;

  void visit(const AccessNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const SlicedAccessNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const LiteralNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const NegNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const SqrtNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const AddNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const SubNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const MulNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const DivNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const MaxNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const MinNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const CastNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const MapNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const CallIntrinsicNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const ReductionNode* op) {
    SUBSTITUTE_EXPR;
  }

  void visit(const AssignmentNode* op) {
    SUBSTITUTE_STMT;
  }

  void visit(const YieldNode* op) {
    SUBSTITUTE_STMT;
  }

  void visit(const ForallNode* op) {
    SUBSTITUTE_STMT;
  }

  void visit(const WhereNode* op) {
    SUBSTITUTE_STMT;
  }

  void visit(const SequenceNode* op) {
    SUBSTITUTE_STMT;
  }

  void visit(const MultiNode* op) {
    SUBSTITUTE_STMT;
  }
};

struct ReplaceIndexVars : public IndexNotationRewriter {
  struct ReplaceIndexVarsInner : public IndexVarExprRewriter {
    const std::map<IndexVar,IndexVar>& substitutions;
    ReplaceIndexVarsInner(const std::map<IndexVar,IndexVar>& substitutions)
        : substitutions(substitutions) {}
  
    using IndexVarExprRewriter::visit;
  
    void visit(const IndexVarAccessNode* op) {
      if (util::contains(substitutions, op->ivar)) {
        expr = substitutions.at(op->ivar);
      }
      else {
        expr = op;
      }
    }
  };

  ReplaceIndexVarsInner indexVarExprRewriter;
  ReplaceIndexVars(const std::map<IndexVar,IndexVar>& substitutions)
      : indexVarExprRewriter(substitutions) {}

  using IndexNotationRewriter::visit;

  void visit(const AccessNode* op) {
    vector<IndexVarExpr> indices;
    bool modified = false;
    for (auto& index : op->indices) {
      const auto rewrittenIndex = indexVarExprRewriter.rewrite(index);
      indices.push_back(rewrittenIndex);
      if (rewrittenIndex != index) {
        modified = true;
      }
    }
    if (modified) {
      expr = Access(op->tensorVar, indices);
    }
    else {
      expr = op;
    }
  }

  void visit(const SlicedAccessNode* op) {
    vector<IndexVar> indices;
    bool modified = false;
    for (auto& index : op->indices) {
      if (util::contains(indexVarExprRewriter.substitutions, index)) {
        indices.push_back(indexVarExprRewriter.substitutions.at(index));
        modified = true;
      }
      else {
        indices.push_back(index);
      }
    }
    if (modified) {
      expr = SlicedAccess(op->tensorVar, indices, op->slicedDims);
    }
    else {
      expr = op;
    }
  }

  // TODO: Replace in assignments
};

struct ReplaceTensorVars : public IndexNotationRewriter {
  const std::map<TensorVar,TensorVar>& substitutions;
  ReplaceTensorVars(const std::map<TensorVar,TensorVar>& substitutions)
      : substitutions(substitutions) {}

  using IndexNotationRewriter::visit;

  void visit(const AccessNode* op) {
    TensorVar var = op->tensorVar;
    expr = (util::contains(substitutions, var))
           ? Access(substitutions.at(var), op->indices)
           : op;
  }

  void visit(const SlicedAccessNode* op) {
    TensorVar var = op->tensorVar;
    expr = (util::contains(substitutions, var))
           ? SlicedAccess(substitutions.at(var), op->indices, op->slicedDims)
           : op;
  }

  void visit(const AssignmentNode* node) {
    TensorVar var = node->lhs.getTensorVar();
    if (util::contains(substitutions, var)) {
      stmt = Assignment(substitutions.at(var),
                        node->lhs.getIndexVars(), rewrite(node->rhs),
                        node->op);
    }
    else {
      IndexNotationRewriter::visit(node);
    }
  }
};

IndexExpr replace(IndexExpr expr,
                  const std::map<IndexExpr,IndexExpr>& substitutions) {
  return ReplaceRewriter(substitutions, {}).rewrite(expr);
}

IndexExpr replace(IndexExpr expr,
                  const std::map<IndexVar,IndexVar>& substitutions) {
  return ReplaceIndexVars(substitutions).rewrite(expr);
}

IndexStmt replace(IndexStmt stmt,
                  const std::map<IndexExpr,IndexExpr>& substitutions) {
  return ReplaceRewriter(substitutions,{}).rewrite(stmt);
}

IndexStmt replace(IndexStmt stmt,
                  const std::map<IndexStmt,IndexStmt>& substitutions) {
  return ReplaceRewriter({}, substitutions).rewrite(stmt);
}

IndexStmt replace(IndexStmt stmt,
                  const std::map<TensorVar,TensorVar>& substitutions) {
  return ReplaceTensorVars(substitutions).rewrite(stmt);
}

}
