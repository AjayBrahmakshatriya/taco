#ifndef TACO_INDEX_NOTATION_NODES_H
#define TACO_INDEX_NOTATION_NODES_H

#include <vector>
#include <memory>

#include "taco/type.h"
#include "taco/index_notation/index_notation.h"
#include "taco/index_notation/index_notation_nodes_abstract.h"
#include "taco/index_notation/index_notation_visitor.h"
#include "taco/index_notation/intrinsic.h"
#include "taco/util/strings.h"

namespace taco {

struct BinaryIndexVarExprNode : public IndexVarExprNode {
  virtual std::string getOperatorString() const = 0;

  IndexVarExpr a;
  IndexVarExpr b;

protected:
  BinaryIndexVarExprNode() = default;
  BinaryIndexVarExprNode(IndexVarExpr a, IndexVarExpr b) : a(a), b(b) {}
};


struct IndexVarAccessNode : public IndexVarExprNode {
  IndexVarAccessNode(IndexVar ivar) : ivar(ivar) {}

  void accept(IndexVarExprVisitorStrict* v) const {
    v->visit(this);
  }

  IndexVar ivar;
};

struct IndexVarLiteralNode : public IndexVarExprNode {
  IndexVarLiteralNode(size_t val) : val(val) {}

  void accept(IndexVarExprVisitorStrict* v) const {
    v->visit(this);
  }

  size_t val;
};


struct IndexVarSubNode : public BinaryIndexVarExprNode {
  IndexVarSubNode() : BinaryIndexVarExprNode() {}
  IndexVarSubNode(IndexVarExpr a, IndexVarExpr b) : BinaryIndexVarExprNode(a, b) {}

  std::string getOperatorString() const {
    return "-";
  }

  void accept(IndexVarExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct IndexVarDivNode : public BinaryIndexVarExprNode {
  IndexVarDivNode() : BinaryIndexVarExprNode() {}
  IndexVarDivNode(IndexVarExpr a, IndexVarExpr b) : BinaryIndexVarExprNode(a, b) {}

  std::string getOperatorString() const {
    return "/";
  }

  void accept(IndexVarExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct IndexVarCountNode : public IndexVarExprNode {
  IndexVarCountNode(const std::vector<IndexVar>& indexVars) : indexVars(indexVars) {}

  void accept(IndexVarExprVisitorStrict* v) const {
    v->visit(this);
  }

  std::vector<IndexVar> indexVars;
};


struct AccessNode : public IndexExprNode {
  AccessNode(TensorVar tensorVar, const std::vector<IndexVar>& indices)
      : IndexExprNode(tensorVar.getType().getDataType()), tensorVar(tensorVar) {
    for (const auto& ivar : indices) {
      this->indices.push_back(ivar);
    }
  }
  AccessNode(TensorVar tensorVar, const std::vector<IndexVarExpr>& indices)
      : IndexExprNode(tensorVar.getType().getDataType()), tensorVar(tensorVar), indices(indices) {}

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }

  virtual void setAssignment(const Assignment& assignment) {}

  TensorVar tensorVar;
  std::vector<IndexVarExpr> indices;
};

struct SlicedAccessNode : public IndexExprNode {
  SlicedAccessNode(TensorVar tensorVar, const std::vector<IndexVar>& indices,
                   const std::vector<bool>& slicedDims)
      : IndexExprNode(tensorVar.getType().getDataType()), 
        tensorVar(tensorVar), indices(indices), slicedDims(slicedDims) {
    taco_iassert(indices.size() == slicedDims.size());
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }

  TensorVar tensorVar;
  std::vector<IndexVar> indices;
  std::vector<bool> slicedDims;
};

struct LiteralNode : public IndexExprNode {
  template <typename T> LiteralNode(T val) : IndexExprNode(type<T>()) {
    this->val = malloc(sizeof(T));
    *static_cast<T*>(this->val) = val;
  }

  ~LiteralNode() {
    free(val);
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }

  template <typename T> T getVal() const {
    taco_iassert(getDataType() == type<T>())
        << "Attempting to get data of wrong type";
    return *static_cast<T*>(val);
  }

  void* val;
};


struct UnaryExprNode : public IndexExprNode {
  IndexExpr a;

protected:
  UnaryExprNode(IndexExpr a) : IndexExprNode(a.getDataType()), a(a) {}
};


struct NegNode : public UnaryExprNode {
  NegNode(IndexExpr operand) : UnaryExprNode(operand) {}

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct BinaryExprNode : public IndexExprNode {
  virtual std::string getOperatorString() const = 0;

  IndexExpr a;
  IndexExpr b;

protected:
  BinaryExprNode() : IndexExprNode() {}
  BinaryExprNode(IndexExpr a, IndexExpr b)
      : IndexExprNode(max_type(a.getDataType(), b.getDataType())), a(a), b(b) {}
};


struct AddNode : public BinaryExprNode {
  AddNode() : BinaryExprNode() {}
  AddNode(IndexExpr a, IndexExpr b) : BinaryExprNode(a, b) {}

  std::string getOperatorString() const {
    return "+";
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct SubNode : public BinaryExprNode {
  SubNode() : BinaryExprNode() {}
  SubNode(IndexExpr a, IndexExpr b) : BinaryExprNode(a, b) {}

  std::string getOperatorString() const {
    return "-";
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct MulNode : public BinaryExprNode {
  MulNode() : BinaryExprNode() {}
  MulNode(IndexExpr a, IndexExpr b) : BinaryExprNode(a, b) {}

  std::string getOperatorString() const {
    return "*";
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct DivNode : public BinaryExprNode {
  DivNode() : BinaryExprNode() {}
  DivNode(IndexExpr a, IndexExpr b) : BinaryExprNode(a, b) {}

  std::string getOperatorString() const {
    return "/";
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct MaxNode : public BinaryExprNode {
  MaxNode() : BinaryExprNode() {}
  MaxNode(IndexExpr a, IndexExpr b) : BinaryExprNode(a, b) {}

  std::string getOperatorString() const {
    return "max";
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct MinNode : public BinaryExprNode {
  MinNode() : BinaryExprNode() {}
  MinNode(IndexExpr a, IndexExpr b) : BinaryExprNode(a, b) {}

  std::string getOperatorString() const {
    return "min";
  }

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }
};


struct SqrtNode : public UnaryExprNode {
  SqrtNode(IndexExpr operand) : UnaryExprNode(operand) {}

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }

};


struct CastNode : public IndexExprNode {
  CastNode(IndexExpr operand, Datatype newType);

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }

  IndexExpr a;
};


struct MapNode : public IndexExprNode {
  MapNode(IndexExpr in, IndexExpr out) : in(in), out(out) {}

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }

  IndexExpr in;
  IndexExpr out;
};


struct CallIntrinsicNode : public IndexExprNode {
  CallIntrinsicNode(const std::shared_ptr<Intrinsic>& func,
                    const std::vector<IndexExpr>& args); 

  void accept(IndexExprVisitorStrict* v) const {
    v->visit(this);
  }

  std::shared_ptr<Intrinsic> func;
  std::vector<IndexExpr> args;
};


struct ReductionNode : public IndexExprNode {
  ReductionNode(IndexExpr op, IndexVar var, IndexExpr a);

  void accept(IndexExprVisitorStrict* v) const {
     v->visit(this);
  }

  IndexExpr op;  // The binary reduction operator, which is a `BinaryExprNode`
                 // with undefined operands)
  IndexVar var;
  IndexExpr a;
};


// Index Statements
struct AssignmentNode : public IndexStmtNode {
  AssignmentNode(const Access& lhs, const IndexExpr& rhs, const IndexExpr& op)
      : lhs(lhs), rhs(rhs), op(op) {}

  void accept(IndexStmtVisitorStrict* v) const {
    v->visit(this);
  }

  Access    lhs;
  IndexExpr rhs;
  IndexExpr op;
};

struct YieldNode : public IndexStmtNode {
  YieldNode(const std::vector<IndexVar>& indexVars, IndexExpr expr)
      : indexVars(indexVars), expr(expr) {}

  void accept(IndexStmtVisitorStrict* v) const {
    v->visit(this);
  }

  std::vector<IndexVar> indexVars;
  IndexExpr expr;
};

struct ForallNode : public IndexStmtNode {
  ForallNode(IndexVar indexVar, IndexStmt stmt, std::set<Forall::TAG> tags)
      : indexVar(indexVar), stmt(stmt), tags(tags) {}

  void accept(IndexStmtVisitorStrict* v) const {
    v->visit(this);
  }

  IndexVar indexVar;
  IndexStmt stmt;
  std::set<Forall::TAG> tags;
};

struct WhereNode : public IndexStmtNode {
  WhereNode(IndexStmt consumer, IndexStmt producer)
      : consumer(consumer), producer(producer) {}

  void accept(IndexStmtVisitorStrict* v) const {
    v->visit(this);
  }

  IndexStmt consumer;
  IndexStmt producer;
};

struct MultiNode : public IndexStmtNode {
  MultiNode(IndexStmt stmt1, IndexStmt stmt2) : stmt1(stmt1), stmt2(stmt2) {}

  void accept(IndexStmtVisitorStrict* v) const {
    v->visit(this);
  }

  IndexStmt stmt1;
  IndexStmt stmt2;
};

struct SequenceNode : public IndexStmtNode {
  SequenceNode(IndexStmt definition, IndexStmt mutation)
      : definition(definition), mutation(mutation) {}

  void accept(IndexStmtVisitorStrict* v) const {
    v->visit(this);
  }

  IndexStmt definition;
  IndexStmt mutation;
};


/// Returns true if index var expression e is of type E.
template <typename E>
inline bool isa(const IndexVarExprNode* e) {
  return e != nullptr && dynamic_cast<const E*>(e) != nullptr;
}

/// Casts the index var expression e to type E.
template <typename E>
inline const E* to(const IndexVarExprNode* e) {
  taco_iassert(isa<E>(e)) <<
      "Cannot convert " << typeid(e).name() << " to " << typeid(E).name();
  return static_cast<const E*>(e);
}

/// Returns true if expression e is of type E.
template <typename E>
inline bool isa(const IndexExprNode* e) {
  return e != nullptr && dynamic_cast<const E*>(e) != nullptr;
}

/// Casts the expression e to type E.
template <typename E>
inline const E* to(const IndexExprNode* e) {
  taco_iassert(isa<E>(e)) <<
      "Cannot convert " << typeid(e).name() << " to " << typeid(E).name();
  return static_cast<const E*>(e);
}

/// Returns true if statement e is of type S.
template <typename S>
inline bool isa(const IndexStmtNode* s) {
  return s != nullptr && dynamic_cast<const S*>(s) != nullptr;
}

/// Casts the index statement node s to subtype S.
template <typename SubType>
inline const SubType* to(const IndexStmtNode* s) {
  taco_iassert(isa<SubType>(s)) <<
      "Cannot convert " << typeid(s).name() << " to " << typeid(SubType).name();
  return static_cast<const SubType*>(s);
}

template <typename I>
inline const typename I::Node* getNode(const I& stmt) {
  taco_iassert(isa<typename I::Node>(stmt.ptr));
  return static_cast<const typename I::Node*>(stmt.ptr);
}

}
#endif
