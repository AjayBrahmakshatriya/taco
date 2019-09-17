#ifndef TACO_INDEX_NOTATION_REWRITER_H
#define TACO_INDEX_NOTATION_REWRITER_H

#include <map>

#include "taco/index_notation/index_notation.h"
#include "taco/index_notation/index_notation_visitor.h"

namespace taco {


/// Extend this class to rewrite all index var expressions.
class IndexVarExprRewriterStrict : public IndexVarExprVisitorStrict {
public:
  virtual ~IndexVarExprRewriterStrict() {}

  /// Rewrite an index var expression.
  IndexVarExpr rewrite(IndexVarExpr);

protected:
  /// Assign to expr in visit methods to replace the visited expr.
  IndexVarExpr expr;

  using IndexVarExprVisitorStrict::visit;

  virtual void visit(const IndexVarAccessNode*) = 0;
  virtual void visit(const IndexVarLiteralNode*) = 0;
  virtual void visit(const IndexVarSubNode*) = 0;
  virtual void visit(const IndexVarDivNode*) = 0;
  virtual void visit(const IndexVarCountNode*) = 0;
};


/// Extend this class to rewrite some index expressions and statements.
class IndexVarExprRewriter : public IndexVarExprRewriterStrict {
public:
  virtual ~IndexVarExprRewriter() {}

protected:
  using IndexVarExprRewriterStrict::visit;

  virtual void visit(const IndexVarAccessNode* op);
  virtual void visit(const IndexVarLiteralNode* op);
  virtual void visit(const IndexVarSubNode* op);
  virtual void visit(const IndexVarDivNode* op);
  virtual void visit(const IndexVarCountNode* op);
};


/// Extend this class to rewrite all index expressions.
class IndexExprRewriterStrict : public IndexExprVisitorStrict {
public:
  virtual ~IndexExprRewriterStrict() {}

  /// Rewrite an index expression.
  IndexExpr rewrite(IndexExpr);

protected:
  /// Assign to expr in visit methods to replace the visited expr.
  IndexExpr expr;

  using IndexExprVisitorStrict::visit;

  virtual void visit(const AccessNode* op) = 0;
  virtual void visit(const SlicedAccessNode*) = 0;
  virtual void visit(const LiteralNode* op) = 0;
  virtual void visit(const NegNode* op) = 0;
  virtual void visit(const SqrtNode* op) = 0;
  virtual void visit(const AddNode* op) = 0;
  virtual void visit(const SubNode* op) = 0;
  virtual void visit(const MulNode* op) = 0;
  virtual void visit(const DivNode* op) = 0;
  virtual void visit(const MaxNode*) = 0;
  virtual void visit(const MinNode*) = 0;
  virtual void visit(const CastNode* op) = 0;
  virtual void visit(const MapNode*) = 0;
  virtual void visit(const CallIntrinsicNode* op) = 0;
  virtual void visit(const ReductionNode* op) = 0;
};


/// Extend this class to rewrite all index statements.
class IndexStmtRewriterStrict : public IndexStmtVisitorStrict {
public:
  virtual ~IndexStmtRewriterStrict() {}

  /// Rewrite an index statement.
  IndexStmt rewrite(IndexStmt);

protected:
  /// Assign to stmt in visit methods to replace the visited stmt.
  IndexStmt stmt;

  using IndexStmtVisitorStrict::visit;

  virtual void visit(const AssignmentNode* op) = 0;
  virtual void visit(const YieldNode* op) = 0;
  virtual void visit(const ForallNode* op) = 0;
  virtual void visit(const WhereNode* op) = 0;
  virtual void visit(const SequenceNode* op) = 0;
  virtual void visit(const MultiNode* op) = 0;
};


/// Extend this class to rewrite all index expressions and statements.
class IndexNotationRewriterStrict : public IndexExprRewriterStrict,
                                    public IndexStmtRewriterStrict {
public:
  virtual ~IndexNotationRewriterStrict() {}

  using IndexExprRewriterStrict::rewrite;
  using IndexStmtRewriterStrict::rewrite;

protected:
  using IndexExprRewriterStrict::visit;
  using IndexStmtRewriterStrict::visit;
};


/// Extend this class to rewrite some index expressions and statements.
class IndexNotationRewriter : public IndexNotationRewriterStrict {
public:
  virtual ~IndexNotationRewriter() {}

protected:
  using IndexNotationRewriterStrict::visit;

  virtual void visit(const AccessNode* op);
  virtual void visit(const SlicedAccessNode* node);
  virtual void visit(const LiteralNode* op);
  virtual void visit(const NegNode* op);
  virtual void visit(const SqrtNode* op);
  virtual void visit(const AddNode* op);
  virtual void visit(const SubNode* op);
  virtual void visit(const MulNode* op);
  virtual void visit(const DivNode* op);
  virtual void visit(const MaxNode* node);
  virtual void visit(const MinNode* node);
  virtual void visit(const CastNode* op);
  virtual void visit(const MapNode* node);
  virtual void visit(const CallIntrinsicNode* op);
  virtual void visit(const ReductionNode* op);

  virtual void visit(const AssignmentNode* op);
  virtual void visit(const YieldNode* op);
  virtual void visit(const ForallNode* op);
  virtual void visit(const WhereNode* op);
  virtual void visit(const SequenceNode* op);
  virtual void visit(const MultiNode* op);
};


/// Rewrites the expression to replace sub-expressions with new expressions.
IndexExpr replace(IndexExpr expr,
                  const std::map<IndexExpr,IndexExpr>& substitutions);

/// Rewrites the expression to replace an index variable with a new variable.
IndexExpr replace(IndexExpr expr,
                  const std::map<IndexVar,IndexVar>& substitutions);

/// Rewrites the statement to replace expressions.
IndexStmt replace(IndexStmt stmt,
                  const std::map<IndexExpr,IndexExpr>& substitutions);

/// Rewrites the statement to replace statements.
IndexStmt replace(IndexStmt stmt,
                  const std::map<IndexStmt,IndexStmt>& substitutions);

/// Rewrites the statement to replace tensor variables.
IndexStmt replace(IndexStmt stmt,
                  const std::map<TensorVar,TensorVar>& substitutions);

}
#endif
