#pragma once

#include "../queryman/ValueHolder.h"
#include <cstring>

struct SetClause {
    string attrName;
    ValueHolder val;
    SetClause(string attrName, ValueHolder v)
    {
        this->attrName = attrName;
        this->val = v;
    }
};