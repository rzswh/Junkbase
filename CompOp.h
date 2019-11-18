#pragma once
#include <cstring>
#include "def.h"

class CompOp {
    AttrType attrType;
public:
    CompOp(AttrType attrType):attrType(attrType) {}

    virtual bool checkDefault(const void *a, const void *b, int) {
        return false;
    }
    virtual bool checkInt(const void *a, const void *b, int l) {
        return checkDefault(a, b, l);
    }
    virtual bool checkDemical(const void *a, const void *b, int l) {
        return checkDefault(a, b, l);
    }
    virtual bool checkChar(const void *a, const void *b, int l) {
        return checkDefault(a, b, l);
    }
    virtual bool checkVarchar(const void *a, const void *b, int l) {
        return checkDefault(a, b, l);
    }
    virtual bool checkDate(const void *a, const void *b, int l) {
        return checkDefault(a, b, l);
    }

    bool check(const void *a, const void *b, int len) { return checkWithType(a, b, len, attrType); }
    bool checkWithType(const void *a, const void *b, int len, AttrType attrType) {
        bool retCode = false;
        switch (attrType)
        {
        case TYPE_INT:
            retCode = checkInt(a, b, 4);
            break;
        case TYPE_NUMERIC:
            retCode = checkDemical(a, b, len);
            break;
        case TYPE_CHAR:
            retCode = checkChar(a, b, len);
            break;
        case TYPE_VARCHAR:
            retCode = checkVarchar(a, b, len);
            break;
        case TYPE_DATE:
            retCode = checkDate(a, b, len);
            break;
        default:
            break;
        }
    }
};

class CompOpBin : public CompOp {
public:
    CompOpBin(AttrType attrType):CompOp(attrType) {}
};

class Every : public CompOpBin {
public:
    Every(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b, int) {
        return true;
    }
};

class Equal : public CompOpBin {
public:
    Equal(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b, int l) {
        return memcmp(a, b, l) == 0;
    }
};

class NotEqual : public CompOpBin {
public:
    NotEqual(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b , int l) {
        return memcmp(a, b, l) != 0;
    }
};

class Less : public CompOpBin {
public:
    Less(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkInt(const void *a, const void *b , int l) override {
        return *((int*)a) < *((int*)b);
    }
    virtual bool checkDefault(const void *a, const void *b , int l) override {
        return memcmp(a, b, l) < 0;
    }
};

class Greater : public CompOpBin {
public:
    Greater(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkInt(const void *a, const void *b , int l) override {
        return *((int*)a) > *((int*)b);
    }
    virtual bool checkDefault(const void *a, const void *b , int l) override {
        return memcmp(a, b, l) > 0;
    }
};

class Operation {
public:
    enum OpKind {EVERY, EQUAL, NEQUAL, LESS, GREATER};
    const OpKind codeOp;
    const AttrType attrType;
    Operation (OpKind op, AttrType type) : codeOp(op), attrType(type) {}
    bool check(const void *a, const void *b, int len) const {
        CompOp * obj;
        switch (codeOp)
        {
        case EVERY: 
            obj = new Every(attrType);
            break;
        case EQUAL: 
            obj = new Equal(attrType);
            break;
        case NEQUAL: 
            obj = new NotEqual(attrType);
            break;
        case LESS: 
            obj = new Less(attrType);
            break;
        case GREATER: 
            obj = new Greater(attrType);
            break;
        default: return false;
        }
        bool retCode = obj->check(a, b, len);
        delete obj;
        return retCode;
    }
};
