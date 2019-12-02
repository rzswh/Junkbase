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
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res) {
        return false;
    }

    bool check(const void *a, const void *b, int len) { return checkWithType(a, b, len, attrType); }
    bool checkWithType(const void *a, const void *b, int len, AttrType attrType) {
        bool retCode = false;
        int type = attrType;
        if (attrType & 8) {
            len -= 8;
            type -= 8;
        }
        switch (type)
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
        if (attrType & 8)
            retCode = checkWithRID(a, b, len, retCode);
        return retCode;
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
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res) {
        return true;
    }
};

class Equal : public CompOpBin {
public:
    Equal(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b, int l) {
        return memcmp(a, b, l) == 0;
    }
    virtual bool checkChar(const void *a, const void *b, int l) {
        return strcmp((char*)a, (char*)b) == 0;
    }
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res) {
        return res && memcmp((char*)a + l, (char*)b + l, sizeof(RID)) == 0;
    }
};

class NotEqual : public CompOpBin {
public:
    NotEqual(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b , int l) {
        return memcmp(a, b, l) != 0;
    }
    virtual bool checkChar(const void *a, const void *b, int l) {
        return strcmp((char*)a, (char*)b) != 0;
    }
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res) {
        return res || memcmp((char*)a + l, (char*)b + l, sizeof(RID)) != 0;
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
    virtual bool checkChar(const void *a, const void *b, int l) {
        return strcmp((char*)a, (char*)b) < 0;
    }
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res) {
        const RID* ar = (RID*)((char*)a + l);
        const RID* br = (RID*)((char*)b + l);
        return res || memcmp(a, b, l) == 0 && *ar < *br;
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
    virtual bool checkChar(const void *a, const void *b, int l) {
        return strcmp((char*)a, (char*)b) > 0;
    }
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res) {
        const RID* ar = (RID*)((char*)a + l);
        const RID* br = (RID*)((char*)b + l);
        return res || memcmp(a, b, l) == 0 && *br < *ar;
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
