#include "taco/index_notation/index_notation_printer.h"
#include "taco/index_notation/index_notation_nodes.h"

using namespace std;

namespace taco {

IndexVarExprPrinter::IndexVarExprPrinter(std::ostream& os) : os(os) {
}

void IndexVarExprPrinter::print(const IndexVarExpr& expr) {
  parentPrecedence = Precedence::TOP;
  expr.accept(this);
}

void IndexVarExprPrinter::visit(const IndexVarAccessNode* op) {
  os << op->ivar;
}

void IndexVarExprPrinter::visit(const IndexVarLiteralNode* op) {
  os << op->val;
}

template <typename Node>
void IndexVarExprPrinter::visitBinary(Node op, Precedence precedence) {
  bool parenthesize =  precedence > parentPrecedence;
  if (parenthesize) {
    os << "(";
  }
  parentPrecedence = precedence;
  op->a.accept(this);
  os << " " << op->getOperatorString() << " ";
  parentPrecedence = precedence;
  op->b.accept(this);
  if (parenthesize) {
    os << ")";
  }
}

void IndexVarExprPrinter::visit(const IndexVarAddNode* op) {
  visitBinary(op, Precedence::ADD);
}

void IndexVarExprPrinter::visit(const IndexVarSubNode* op) {
  visitBinary(op, Precedence::SUB);
}

void IndexVarExprPrinter::visit(const IndexVarDivNode* op) {
  visitBinary(op, Precedence::DIV);
}

void IndexVarExprPrinter::visit(const IndexVarCountNode* op) {
  os << "count(" << util::join(op->indexVars, ",") << ")";
}

IndexNotationPrinter::IndexNotationPrinter(std::ostream& os) : os(os) {
}

void IndexNotationPrinter::print(const IndexExpr& expr) {
  parentPrecedence = Precedence::TOP;
  expr.accept(this);
}

void IndexNotationPrinter::print(const IndexStmt& expr) {
  parentPrecedence = Precedence::TOP;
  expr.accept(this);
}

void IndexNotationPrinter::visit(const AccessNode* op) {
  os << op->tensorVar.getName();
  if (op->indices.size() > 0) {
    os << "(" << util::join(op->indices,",") << ")";
  }
}

void IndexNotationPrinter::visit(const SlicedAccessNode* op) {
  os << op->tensorVar.getName();
  if (op->indices.size() > 0) {
    os << "(";
    std::string sep = "";
    for (size_t i = 0; i < op->indices.size(); ++i) {
      os << sep;
      if (op->slicedDims[i]) {
        os << ":";
      } else {
        os << op->indices[i];
      }
      sep = ", ";
    }
    os << ")";
  }
}

void IndexNotationPrinter::visit(const LiteralNode* op) {
  switch (op->getDataType().getKind()) {
    case Datatype::Bool:
      os << op->getVal<bool>();
      break;
    case Datatype::UInt8:
      os << op->getVal<uint8_t>();
      break;
    case Datatype::UInt16:
      os << op->getVal<uint16_t>();
      break;
    case Datatype::UInt32:
      os << op->getVal<uint32_t>();
      break;
    case Datatype::UInt64:
      os << op->getVal<uint64_t>();
      break;
    case Datatype::UInt128:
      taco_not_supported_yet;
      break;
    case Datatype::Int8:
      os << op->getVal<int8_t>();
      break;
    case Datatype::Int16:
      os << op->getVal<int16_t>();
      break;
    case Datatype::Int32:
      os << op->getVal<int32_t>();
      break;
    case Datatype::Int64:
      os << op->getVal<int64_t>();
      break;
    case Datatype::Int128:
      taco_not_supported_yet;
      break;
    case Datatype::Float32:
      os << op->getVal<float>();
      break;
    case Datatype::Float64:
      os << op->getVal<double>();
      break;
    case Datatype::Complex64:
      os << op->getVal<std::complex<float>>();
      break;
    case Datatype::Complex128:
      os << op->getVal<std::complex<double>>();
      break;
    case Datatype::Undefined:
      break;
  }
}

void IndexNotationPrinter::visit(const NegNode* op) {
  Precedence precedence = Precedence::NEG;
  bool parenthesize =  precedence > parentPrecedence;
  parentPrecedence = precedence;
  os << "-";
  if (parenthesize) {
    os << "(";
  }
  op->a.accept(this);
  if (parenthesize) {
    os << ")";
  }
}

void IndexNotationPrinter::visit(const SqrtNode* op) {
  parentPrecedence = Precedence::FUNC;
  os << "sqrt";
  os << "(";
  op->a.accept(this);
  os << ")";
}

template <typename Node>
void IndexNotationPrinter::visitBinary(Node op, Precedence precedence) {
  bool parenthesize =  precedence > parentPrecedence;
  if (parenthesize) {
    os << "(";
  }
  parentPrecedence = precedence;
  op->a.accept(this);
  os << " " << op->getOperatorString() << " ";
  parentPrecedence = precedence;
  op->b.accept(this);
  if (parenthesize) {
    os << ")";
  }
}

void IndexNotationPrinter::visit(const AddNode* op) {
  visitBinary(op, Precedence::ADD);
}

void IndexNotationPrinter::visit(const SubNode* op) {
  visitBinary(op, Precedence::SUB);
}

void IndexNotationPrinter::visit(const MulNode* op) {
  visitBinary(op, Precedence::MUL);
}

void IndexNotationPrinter::visit(const DivNode* op) {
  visitBinary(op, Precedence::DIV);
}

void IndexNotationPrinter::visit(const MaxNode* op) {
  parentPrecedence = Precedence::FUNC;
  os << "max(";
  op->a.accept(this);
  os << ", ";
  op->b.accept(this);
  os << ")";
}

void IndexNotationPrinter::visit(const MinNode* op) {
  parentPrecedence = Precedence::FUNC;
  os << "min(";
  op->a.accept(this);
  os << ", ";
  op->b.accept(this);
  os << ")";
}

void IndexNotationPrinter::visit(const CastNode* op) {
  parentPrecedence = Precedence::CAST;
  os << "cast<";
  os << op->getDataType();
  os << ">(";
  op->a.accept(this);
  os << ")";
}

void IndexNotationPrinter::visit(const MapNode* op) {
  parentPrecedence = Precedence::FUNC;
  os << "map(";
  op->in.accept(this);
  os << ", ";
  op->out.accept(this);
  os << ")";
}

template <class T>
static inline void acceptJoin(IndexNotationPrinter* printer, 
                              std::ostream& stream, const std::vector<T>& nodes, 
                              std::string sep) {
  if (nodes.size() > 0) {
    nodes[0].accept(printer);
  }
  for (size_t i = 1; i < nodes.size(); ++i) {
    stream << sep;
    nodes[i].accept(printer);
  }
}

void IndexNotationPrinter::visit(const CallIntrinsicNode* op) {
  parentPrecedence = Precedence::FUNC;
  os << op->func->getName();
  os << "(";
  acceptJoin(this, os, op->args, ", ");
  os << ")";
}

void IndexNotationPrinter::visit(const ReductionNode* op) {
  struct ReductionName : IndexNotationVisitor {
    std::string reductionName;
    std::string get(IndexExpr expr) {
      expr.accept(this);
      return reductionName;
    }
    using IndexNotationVisitor::visit;
    void visit(const AddNode* node) {
      reductionName = "sum";
    }
    void visit(const MulNode* node) {
      reductionName = "product";
    }
    void visit(const BinaryExprNode* node) {
      reductionName = "reduction(" + node->getOperatorString() + ")";
    }
  };
  parentPrecedence = Precedence::REDUCTION;
  os << ReductionName().get(op->op) << "(" << op->var << ", ";
  op->a.accept(this);
  os << ")";
}

void IndexNotationPrinter::visit(const AssignmentNode* op) {
  struct OperatorName : IndexNotationVisitor {
    using IndexNotationVisitor::visit;
    std::string operatorName;
    std::string get(IndexExpr expr) {
      if (!expr.defined()) return "";
      expr.accept(this);
      return operatorName;
    }
    void visit(const BinaryExprNode* node) {
      operatorName = node->getOperatorString();
    }
  };

  op->lhs.accept(this);
  os << " " << OperatorName().get(op->op) << "= ";
  op->rhs.accept(this);
}

void IndexNotationPrinter::visit(const YieldNode* op) {
  os << "yield(";
  if (op->indexVars.size() > 0) {
    os << "{" << util::join(op->indexVars,",") << "}, ";
  }
  op->expr.accept(this);
  os << ")";
}

void IndexNotationPrinter::visit(const ForallNode* op) {
  os << "forall(" << op->indexVar << ", ";
  op->stmt.accept(this);
  for (auto iter = op->tags.begin(); iter != op->tags.end(); ++iter) {
    switch (*iter) {
      case Forall::PARALLELIZE:
        os << ", PARALLELIZE";
        break;
      default:
        taco_ierror;
    }
  }
  os << ")";
}

void IndexNotationPrinter::visit(const WhereNode* op) {
  os << "where(";
  op->consumer.accept(this);
  os << ", ";
  op->producer.accept(this);
  os << ")";
}

void IndexNotationPrinter::visit(const MultiNode* op) {
  os << "multi(";
  op->stmt1.accept(this);
  os << ", ";
  op->stmt2.accept(this);
  os << ")";
}

void IndexNotationPrinter::visit(const SequenceNode* op) {
  os << "sequence(";
  op->definition.accept(this);
  os << ", ";
  op->mutation.accept(this);
  os << ")";
}

}
