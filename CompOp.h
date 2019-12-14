#pragma once
#include <cstring>
#include "def.h"
#include "utils/type.h"

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
    virtual bool checkRID(const void *a, const void *b, int l) {
        return checkDefault(a, b, l);
    }
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res) {
        return false;
    }
    // when this operation is carried out for a compound type, will check go on 
    // to next key given the compare result of this key.
    virtual bool ifNext(bool result) {
        return false;
    }
    virtual bool ifNext(bool result, AttrTypeAtom type, const void * a, const void * b, int l) {
        return ifNext(result);
    }

    bool check(const void *a, const void *b, int len) { return checkWithType(a, b, len, (AttrTypeAtom)(attrType & 0x7)); }
    bool checkCompound(const void *a, const void *b, int *len) { return checkWithTypeCompound(a, b, len, attrType); }
    bool checkWithType(const void *a, const void *b, int len, AttrTypeAtom attrType) {
        bool retCode = false;
        int type = attrType;
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
            retCode = checkDate(a, b, sizeof(Date));
            break;
        case TYPE_RID:
            retCode = checkRID(a, b, sizeof(RID));
            break;
        default:
            break;
        }
        return retCode;
    }
    bool checkWithTypeCompound(const void *a, const void *b, int *len, AttrType attrType)  {
        bool retCode = false;
        for (int i = 0; attrType; i++) {
            retCode = checkWithType(a, b, len[i], (AttrTypeAtom)(attrType & 0x7));
            if (!ifNext(retCode, (AttrTypeAtom)(attrType & 0x7), a, b, len[i])) 
                return retCode;
        }
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
    virtual bool ifNext(bool res) override { return false; }
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
    virtual bool ifNext(bool res) override { return res; }
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
    virtual bool ifNext(bool res) override { return !res; }
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
    virtual bool ifNext(bool res, AttrTypeAtom type, const void * a, const void * b, int l) override { 
        return !res && Equal(type).checkWithType(a, b, l, type); 
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
    virtual bool ifNext(bool res, AttrTypeAtom type, const void * a, const void * b, int l) override { 
        return !res && Equal(type).checkWithType(a, b, l, type); 
    }
};

class Operation {
public:
    enum OpKind {EVERY, EQUAL, NEQUAL, LESS, GREATER};
    const OpKind codeOp;
    const AttrType attrType;
    Operation (OpKind op, AttrType type) : codeOp(op), attrType(type) {}
    CompOp * getCompOp() const {
        switch (codeOp)
        {
        case EVERY: 
            return new Every(attrType);
        case EQUAL: 
            return new Equal(attrType);
        case NEQUAL: 
            return new NotEqual(attrType);
        case LESS: 
            return new Less(attrType);
        case GREATER: 
            return new Greater(attrType);
        default: return nullptr;
        }
    }
    bool check(const void *a, const void *b, int len) const {
        CompOp * obj = getCompOp();
        bool retCode = obj ? obj->check(a, b, len) : false;
        delete obj;
        return retCode;
    }
    bool check(const void *a, const void *b, int *len) const {
        CompOp * obj = getCompOp();
        bool retCode = obj ? obj->checkCompound(a, b, len) : false;
        delete obj;
        return retCode;
    }
};
