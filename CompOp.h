#pragma once
#include <cstring>
#include <cassert>
#include "def.h"
#include "recman/RID.h"
#include "utils/type.h"

class FileHandle;
class CompOp {
    AttrType attrType;
public:
    CompOp(AttrType attrType);

    virtual bool checkDefault(const void *a, const void *b, int);
    virtual bool checkInt(const void *a, const void *b, int l);
    virtual bool checkDemical(const void *a, const void *b, int l);
    virtual bool checkChar(const void *a, const void *b, int l);
    virtual bool checkVarchar(const void *a, const void *b, int l);
    virtual bool checkDate(const void *a, const void *b, int l);
    virtual bool checkRID(const void *a, const void *b, int l);
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res);
    // when this operation is carried out for a compound type, will check go on 
    // to next key given the compare result of this key.
    virtual bool ifNext(bool result);
    virtual bool ifNext(bool result, AttrTypeAtom type, const void * a, const void * b, int l);

    bool check(const void *a, const void *b, int len, const FileHandle * fh);
    bool checkCompound(const void *a, const void *b, int *len, const FileHandle * fh);
    bool checkWithType(const void *a, const void *b, int len, AttrTypeAtom type, const FileHandle * _fh);
    bool checkWithType(const void *a, const void *b, int len, AttrTypeAtom attrType);
    bool checkWithTypeCompound(const void *a, const void *b, int *len, AttrType attrType, const FileHandle * fh);
};

class CompOpBin : public CompOp {
public:
    CompOpBin(AttrType attrType);
};

class Every : public CompOpBin {
public:
    Every(AttrType attrType):CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b, int);
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res);
    virtual bool ifNext(bool res) override;
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
    const FileHandle * varRef;
    Operation (OpKind op, AttrType type, FileHandle * fh = nullptr);
    CompOp * getCompOp() const;
    bool check(const void *a, const void *b, int len) const;
    bool check(const void *a, const void *b, int *len) const;
};
