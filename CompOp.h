#pragma once
#include <cstring>
#include "def.h"

class CompOp {
public:
    virtual bool checkInt(const void *a, const void *b, int) = 0;
    virtual bool checkDemical(const void *a, const void *b, int) = 0;
    virtual bool checkChar(const void *a, const void *b, int) = 0;
    virtual bool checkVarchar(const void *a, const void *b, int) = 0;
    virtual bool checkDate(const void *a, const void *b, int) = 0;
};

class CompOpBin : public CompOp {};

class Every : public CompOpBin {
public:
    virtual bool checkInt(const void *a, const void *b, int) {
        return true;
    }
    virtual bool checkDemical(const void *a, const void *b, int) {
        return true;
    }
    virtual bool checkChar(const void *a, const void *b, int) {
        return true;
    }
    virtual bool checkVarchar(const void *a, const void *b, int) {
        return true;
    }
    virtual bool checkDate(const void *a, const void *b, int) {
        return true;
    }
};

class Equal : public CompOpBin {
public:
    virtual bool checkInt(const void *a, const void *b, int l) {
        return memcmp(a, b, l) == 0;
    }
    virtual bool checkDemical(const void *a, const void *b, int l) {
        return memcmp(a, b, l) == 0;
    }
    virtual bool checkChar(const void *a, const void *b , int l) {
        return memcmp(a, b, l) == 0;
    }
    virtual bool checkVarchar(const void *a, const void *b , int l) {
        return memcmp(a, b, l) == 0;
    }
    virtual bool checkDate(const void *a, const void *b , int l) {
        return memcmp(a, b, l) == 0;
    }
};

class NotEqual : public CompOpBin {
public:
    virtual bool checkInt(const void *a, const void *b , int l) {
        return memcmp(a, b, l) != 0;
    }
    virtual bool checkDemical(const void *a, const void *b , int l) {
        return memcmp(a, b, l) != 0;
    }
    virtual bool checkChar(const void *a, const void *b , int l) {
        return memcmp(a, b, l) != 0;
    }
    virtual bool checkVarchar(const void *a, const void *b , int l) {
        return memcmp(a, b, l) != 0;
    }
    virtual bool checkDate(const void *a, const void *b , int l) {
        return memcmp(a, b, l) != 0;
    }
};

class Less : public CompOpBin {
public:
    virtual bool checkInt(const void *a, const void *b , int l) {
        return *((int*)a) < *((int*)b);
    }
    virtual bool checkDemical(const void *a, const void *b , int l) {
        return memcmp(a, b, l) < 0;
    }
    virtual bool checkChar(const void *a, const void *b , int l) {
        return memcmp(a, b, l) < 0;
    }
    virtual bool checkVarchar(const void *a, const void *b , int l) {
        return memcmp(a, b, l) < 0;
    }
    virtual bool checkDate(const void *a, const void *b , int l) {
        return memcmp(a, b, l) < 0;
    }
};


class Operation {
private:
    int codeOp;
    AttrType attrType;
public:
    enum OpKind {EVERY, EQUAL, NEQUAL, LESS};
    Operation (int op, AttrType type) : codeOp(op), attrType(type) {}
    bool check(const void *a, const void *b, int len) {
        CompOp * obj;
        switch (codeOp)
        {
        case EVERY: 
            obj = new Every();
            break;
        case EQUAL: 
            obj = new Equal();
            break;
        case NEQUAL: 
            obj = new NotEqual();
            break;
        case LESS: 
            obj = new Less();
            break;
        default: return false;
        }
        bool retCode = false;
        switch (attrType)
        {
        case TYPE_INT:
            retCode = obj->checkInt(a, b, 4);
            break;
        case TYPE_NUMERIC:
            retCode = obj->checkDemical(a, b, len);
            break;
        case TYPE_CHAR:
            retCode = obj->checkChar(a, b, len);
            break;
        case TYPE_VARCHAR:
            retCode = obj->checkVarchar(a, b, len);
            break;
        case TYPE_DATE:
            retCode = obj->checkDate(a, b, len);
            break;
        default:
            break;
        }
        delete obj;
        return retCode;
    }
};
