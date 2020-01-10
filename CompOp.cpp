#include "CompOp.h"
#include "recman/RecordManager.h"

bool inline bothNotNull(const void *a, const void *b)
{
    return !isNull(a, TYPE_CHAR) && !isNull(b, TYPE_CHAR);
}
bool inline bothNotNullForInt(const void *a, const void *b)
{
    return !isNull(a, TYPE_INT) && !isNull(b, TYPE_INT);
}

CompOp::CompOp(AttrType attrType) : attrType(attrType) {}

bool CompOp::checkDefault(const void *a, const void *b, int) { return false; }

bool CompOp::checkInt(const void *a, const void *b, int l)
{
    return checkDefault(a, b, l);
}

bool CompOp::checkDemical(const void *a, const void *b, int l)
{
    return checkDefault(a, b, l);
}

bool CompOp::checkChar(const void *a, const void *b, int l)
{
    return checkDefault(a, b, l);
}

bool CompOp::checkVarchar(const void *a, const void *b, int l)
{
    return checkDefault(a, b, l);
}

bool CompOp::checkDate(const void *a, const void *b, int l)
{
    return checkDefault(a, b, l);
}

bool CompOp::checkRID(const void *a, const void *b, int l)
{
    return checkDefault(a, b, l);
}

bool CompOp::checkWithRID(const void *a, const void *b, int l, bool res)
{
    return false;
}

bool CompOp::ifNext(bool result) { return false; }

bool CompOp::ifNext(bool result, AttrTypeAtom type, const void *a,
                    const void *b, int l)
{
    return ifNext(result);
}

bool CompOp::check(const void *a, const void *b, int len, const FileHandle *fh)
{
    return checkWithType(a, b, len, (AttrTypeAtom)(attrType & 0x7), fh);
}

bool CompOp::checkCompound(const void *a, const void *b, int *len,
                           const FileHandle *fh)
{
    return checkWithTypeCompound(a, b, len, attrType, fh);
}

bool CompOp::checkWithType(const void *a, const void *b, int len,
                           AttrTypeAtom type, const FileHandle *_fh)
{
    AttrTypeAtom ta = (AttrTypeAtom)(attrType & 0x7);
    char *dptr_a = (char *)a, *dptr_b = (char *)b;
    bool creation = false;
    FileHandle *fh = const_cast<FileHandle *>(_fh);
    assert(type != TYPE_VARCHAR || fh != nullptr);
    if (ta == TYPE_VARCHAR) {
        dptr_a = new char[len + 1];
        dptr_b = new char[len + 1];
        creation = true;
        fh->getVariant(*(RID *)a, dptr_a, len);
        fh->getVariant(*(RID *)b, dptr_b, len);
    }
    bool retCode = checkWithType(a, b, len, ta);
    if (creation) delete[] dptr_a, dptr_b;
    return retCode;
}

bool CompOp::checkWithType(const void *a, const void *b, int len,
                           AttrTypeAtom attrType)
{
    bool retCode = false;
    int type = attrType;
    switch (type) {
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

bool CompOp::checkWithTypeCompound(const void *a, const void *b, int *len,
                                   AttrType attrType, const FileHandle *fh)
{
    bool retCode = false;
    for (int i = 0; attrType; i++, attrType >>= 3) {
        retCode =
            checkWithType(a, b, len[i], (AttrTypeAtom)(attrType & 0x7), fh);
        if (!ifNext(retCode, (AttrTypeAtom)(attrType & 0x7), a, b, len[i]))
            return retCode;
    }
    return retCode;
}

CompOpBin::CompOpBin(AttrType attrType) : CompOp(attrType) {}

bool Every::checkDefault(const void *a, const void *b, int) { return true; }

bool Every::checkWithRID(const void *a, const void *b, int l, bool res)
{
    return true;
}

bool Every::ifNext(bool res) { return false; }

// Equal

bool Equal::checkDefault(const void *a, const void *b, int l)
{
    return bothNotNull(a, b) && memcmp(a, b, l) == 0;
}
bool Equal::checkChar(const void *a, const void *b, int l)
{
    return bothNotNull(a, b) && strcmp((char *)a, (char *)b) == 0;
}
bool Equal::checkDemical(const void *a, const void *b, int l)
{
    return bothNotNull(a, b) && *(Numeric *)a == *(Numeric *)b;
}
bool Equal::checkInt(const void *a, const void *b, int l)
{
    return bothNotNullForInt(a, b) && memcmp(a, b, sizeof(int)) == 0;
}
bool Equal::checkWithRID(const void *a, const void *b, int l, bool res)
{
    return res && memcmp((char *)a + l, (char *)b + l, sizeof(RID)) == 0;
}
bool Equal::ifNext(bool res) { return res; }
bool Equal::checkWithType(const void *a, const void *b, int l,
                          AttrTypeAtom attrType)
{
    return CompOp::checkWithType(a, b, l, attrType);
}

// NotEqual

bool NotEqual::checkDefault(const void *a, const void *b, int l)
{
    return bothNotNull(a, b) && memcmp(a, b, l) != 0;
}
bool NotEqual::checkChar(const void *a, const void *b, int l)
{
    return bothNotNull(a, b) && strcmp((char *)a, (char *)b) != 0;
}
bool NotEqual::checkDemical(const void *a, const void *b, int l)
{
    return bothNotNull(a, b) && !(*(Numeric *)a == *(Numeric *)b);
}
bool NotEqual::checkInt(const void *a, const void *b, int l)
{
    return bothNotNullForInt(a, b) && memcmp(a, b, sizeof(int)) != 0;
}
bool NotEqual::checkWithRID(const void *a, const void *b, int l, bool res)
{
    return res || memcmp((char *)a + l, (char *)b + l, sizeof(RID)) != 0;
}
bool NotEqual::ifNext(bool res) { return !res; }

// IsNull

bool IsNull::checkDefault(const void *a, const void *b, int l)
{
    return l == 0 || (((unsigned char *)a)[0] & 0x80) != 0;
}
bool IsNull::checkInt(const void *a, const void *b, int l)
{
    return l == 0 || ((unsigned int *)a)[0] == 0x80000000;
}

bool IsNotNull::checkDefault(const void *a, const void *b, int l)
{
    return l == 0 || (((unsigned char *)a)[0] & 0x80) == 0;
}
bool IsNotNull::checkInt(const void *a, const void *b, int l)
{
    return l == 0 || ((unsigned int *)a)[0] != 0x80000000;
}

// Less

bool Less::checkInt(const void *a, const void *b, int l)
{
    return !isNull(b, TYPE_INT) &&
           (isNull(a, TYPE_INT) || *((int *)a) < *((int *)b));
}
bool Less::checkDefault(const void *a, const void *b, int l)
{
    return !isNull(b) && (isNull(a) || memcmp(a, b, l) < 0);
}
bool Less::checkChar(const void *a, const void *b, int l)
{
    return !isNull(b) && (isNull(a) || strcmp((char *)a, (char *)b) < 0);
}
bool Less::checkDemical(const void *a, const void *b, int l)
{
    return !isNull(b) && (isNull(a) || (*(Numeric *)a < *(Numeric *)b));
}
bool Less::checkDate(const void *a, const void *b, int l)
{
    return !isNull(b) && (isNull(a) || (*(Date *)a < *(Date *)b));
}
bool Less::checkWithRID(const void *a, const void *b, int l, bool res)
{
    const RID *ar = (RID *)((char *)a + l);
    const RID *br = (RID *)((char *)b + l);
    return res || memcmp(a, b, l) == 0 && *ar < *br;
}
bool Less::ifNext(bool res, AttrTypeAtom type, const void *a, const void *b,
                  int l)
{
    return !res && Equal(type).checkWithType(a, b, l, type);
}

// Greater

bool Greater::checkInt(const void *a, const void *b, int l)
{
    return !isNull(a, TYPE_INT) &&
           (isNull(b, TYPE_INT) || *((int *)a) > *((int *)b));
}
bool Greater::checkDefault(const void *a, const void *b, int l)
{
    return !isNull(a) && (isNull(b) || memcmp(a, b, l) > 0);
}
bool Greater::checkChar(const void *a, const void *b, int l)
{
    return !isNull(a) && (isNull(b) || strcmp((char *)a, (char *)b) > 0);
}
bool Greater::checkDemical(const void *a, const void *b, int l)
{
    return !isNull(a) && (isNull(b) || (*(Numeric *)b < *(Numeric *)a));
}
bool Greater::checkDate(const void *a, const void *b, int l)
{
    return !isNull(a) && (isNull(b) || (*(Date *)b < *(Date *)a));
}
bool Greater::checkWithRID(const void *a, const void *b, int l, bool res)
{
    const RID *ar = (RID *)((char *)a + l);
    const RID *br = (RID *)((char *)b + l);
    return res || memcmp(a, b, l) == 0 && *br < *ar;
}
bool Greater::ifNext(bool res, AttrTypeAtom type, const void *a, const void *b,
                     int l)
{
    return !res && Equal(type).checkWithType(a, b, l, type);
}

Operation::Operation(OpKind op, AttrType type, FileHandle *fh)
    : codeOp(op), attrType(type), varRef(fh)
{
}

CompOp *Operation::getCompOp() const
{
    switch (codeOp) {
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
    default:
        return nullptr;
    }
}

bool Operation::check(const void *a, const void *b, int *len) const
{
    CompOp *obj = getCompOp();
    bool retCode = obj ? obj->checkCompound(a, b, len, varRef) : false;
    delete obj;
    return retCode;
}

bool Operation::check(const void *a, const void *b, int len) const
{
    CompOp *obj = getCompOp();
    bool retCode = obj ? obj->check(a, b, len, varRef) : false;
    delete obj;
    return retCode;
}