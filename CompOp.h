#pragma once
#include "def.h"
#include "recman/RID.h"
#include "utils/type.h"
#include <cassert>
#include <cstring>
#include <vector>
using std::vector;

class FileHandle;
class CompOp
{
    AttrType attrType;

public:
    CompOp(AttrType attrType);
    AttrType getType() const { return attrType; }

protected:
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
    virtual bool ifNext(bool result, AttrTypeAtom type, const void *a,
                        const void *b, int l);

public:
    virtual bool check(const void *a, const void *b, int len,
                       const FileHandle *fh);
    virtual bool checkCompound(const void *a, const void *b, int *len,
                               const FileHandle *fh);

protected:
    bool checkWithType(const void *a, const void *b, int len, AttrTypeAtom type,
                       const FileHandle *_fh);
    bool checkWithType(const void *a, const void *b, int len,
                       AttrTypeAtom attrType);
    bool checkWithTypeCompound(const void *a, const void *b, int *len,
                               AttrType attrType, const FileHandle *fh);
};

class CompOpBin : public CompOp
{
public:
    CompOpBin(AttrType attrType);
};

class Every : public CompOpBin
{
public:
    Every(AttrType attrType) : CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b, int);
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res);
    virtual bool ifNext(bool res) override;
};

class Equal : public CompOpBin
{
public:
    Equal(AttrType attrType) : CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b, int l);
    virtual bool checkChar(const void *a, const void *b, int l);
    virtual bool checkDemical(const void *a, const void *b, int l);
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res);
    virtual bool checkInt(const void *a, const void *b, int l);
    virtual bool ifNext(bool res) override;
    bool checkWithType(const void *a, const void *b, int l,
                       AttrTypeAtom attrType);
};

class NotEqual : public CompOpBin
{
public:
    NotEqual(AttrType attrType) : CompOpBin(attrType) {}
    virtual bool checkDefault(const void *a, const void *b, int l);
    virtual bool checkChar(const void *a, const void *b, int l);
    virtual bool checkDemical(const void *a, const void *b, int l);
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res);
    virtual bool checkInt(const void *a, const void *b, int l);
    virtual bool ifNext(bool res) override;
};

class IsNull : public CompOp
{
public:
    IsNull(AttrType at) : CompOp(at) {}
    virtual bool checkDefault(const void *a, const void *b, int);
    virtual bool checkInt(const void *a, const void *b, int l);
};

class IsNotNull : public CompOp
{
public:
    IsNotNull(AttrType at) : CompOp(at) {}
    virtual bool checkDefault(const void *a, const void *b, int);
    virtual bool checkInt(const void *a, const void *b, int l);
};

class Less : public CompOpBin
{
public:
    Less(AttrType attrType) : CompOpBin(attrType) {}
    virtual bool checkInt(const void *a, const void *b, int l) override;
    virtual bool checkDefault(const void *a, const void *b, int l) override;
    virtual bool checkChar(const void *a, const void *b, int l);
    virtual bool checkDemical(const void *a, const void *b, int l);
    virtual bool checkDate(const void *a, const void *b, int l);
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res);
    virtual bool ifNext(bool res, AttrTypeAtom type, const void *a,
                        const void *b, int l) override;
};

class Greater : public CompOpBin
{
public:
    Greater(AttrType attrType) : CompOpBin(attrType) {}
    virtual bool checkInt(const void *a, const void *b, int l) override;
    virtual bool checkDefault(const void *a, const void *b, int l) override;
    virtual bool checkChar(const void *a, const void *b, int l);
    virtual bool checkDemical(const void *a, const void *b, int l);
    virtual bool checkDate(const void *a, const void *b, int l);
    virtual bool checkWithRID(const void *a, const void *b, int l, bool res);
    virtual bool ifNext(bool res, AttrTypeAtom type, const void *a,
                        const void *b, int l) override;
};

template <class T> class NotOp : public T
{
public:
    NotOp(AttrType attrType) : T(attrType) {}
    bool check(const void *a, const void *b, int len,
               const FileHandle *fh) override
    {
        return !T::check(a, b, len, fh);
    }
    bool checkCompound(const void *a, const void *b, int *len,
                       const FileHandle *fh) override
    {
        return !T::checkCompound(a, b, len, fh);
    }
};

template class NotOp<Less>;
template class NotOp<Greater>;
typedef NotOp<Less> GreaterEqual;
typedef NotOp<Greater> LessEqual;

class Operation
{
public:
    enum OpKind { EVERY, EQUAL, NEQUAL, LESS, GREATER };
    const OpKind codeOp;
    const AttrType attrType;
    const FileHandle *varRef;
    Operation(OpKind op, AttrType type, FileHandle *fh = nullptr);
    CompOp *getCompOp() const;
    bool check(const void *a, const void *b, int len) const;
    bool check(const void *a, const void *b, int *len) const;
};
struct ComposedOperation {
    int num;
    vector<Operation> oprs;
    vector<int> offsets;
    vector<int> lengths;
    vector<void *> data;
    ComposedOperation(int n, vector<Operation> oprs, vector<int> off,
                      vector<int> len, vector<void *> data)
        : num(n), oprs(oprs), offsets(off), lengths(len), data(data)
    {
    }
};