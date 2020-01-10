#pragma once

#include "RelAttr.h"
#include "ValueHolder.h"
#include <string>
#include <vector>
using std::string;
using std::vector;

enum WhereCompareOp {
    WO_EQUAL,
    WO_NEQUAL,
    WO_LESS,
    WO_LE,
    WO_GE,
    WO_GREATER,
    WO_ISNULL,
    WO_NOTNULL,
};

class Judge
{
  public:
    RelAttr lhs, rhs;
    ValueHolder valRhs;
    WhereCompareOp op;
    Judge(RelAttr lhs, WhereCompareOp op) : lhs(lhs), op(op) {}
    Judge(RelAttr lhs, WhereCompareOp op, ValueHolder val)
        : lhs(lhs), valRhs(val), op(op)
    {
    }
    Judge(RelAttr lhs, WhereCompareOp op, RelAttr rhs)
        : lhs(lhs), rhs(rhs), op(op)
    {
    }
};

class Condition
{
  public:
    vector<Judge> judSet;
};
