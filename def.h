#pragma once

// #define IF_RET_ERROR(S, R) if ((R = (S)) == 0)

enum AttrTypeAtom {
    TYPE_CHAR = 1,
    TYPE_VARCHAR = 2,
    TYPE_NUMERIC = 3,
    TYPE_INT = 4,
    TYPE_DATE = 5,
    TYPE_RID = 6,
    // internal type
    // TYPE_CHR_RID = 9,
    // TYPE_VCH_RID = 10,
    // TYPE_NMR_RID = 11,
    // TYPE_INT_RID = 12,
    // TYPE_DAT_RID = 13
};

// compound type
// lower bit has higher priority
typedef unsigned int AttrType;

class AttrTypeHelper
{
public:
    static AttrType makeLowestKey(AttrTypeAtom target, AttrType pattern)
    {
        AttrType ret = target;
        while (pattern)
            pattern >>= 3, ret <<= 3;
        return ret;
    }
    static AttrType getLowestKey(AttrType key)
    {
        while (key > 0x7)
            key >>= 3;
        return key;
    }
    static int countKey(AttrType type)
    {
        int ret = 0;
        for (; type; type >>= 3)
            ret++;
        return ret;
    }
    static int getRawLength(int len, AttrTypeAtom atom);

    static bool checkTypeCompliable(AttrTypeAtom attrType, AttrTypeAtom valType)
    {
        return attrType == valType ||
               (attrType == TYPE_VARCHAR && valType == TYPE_CHAR) ||
               (attrType == TYPE_CHAR && valType == TYPE_VARCHAR);
    }
};

struct AttrTypeComplex {
    AttrTypeAtom type;
    int length;
    int M, D; // for muneric
    AttrTypeComplex()
    {
        type = TYPE_CHAR;
        M = D = length = 0;
    }
    AttrTypeComplex(AttrTypeAtom type, int len)
        : type(type), length(len), M(0), D(0)
    {
    }
    AttrTypeComplex(AttrTypeAtom type, int M, int D)
        : type(type), length(40), M(M), D(D)
    {
    }
};

bool isNull(const void *buf, AttrTypeAtom type = TYPE_CHAR);

const int VARIANT_SEGMENT_LENGTH = 64;
const int MAX_TABLE_NAME_LEN = 30;
const int MAX_ATTR_NAME_LEN = 20;
const int MAX_KEY_NAME_LEN = 20;
const int MAX_KEY_COMBIN_NUM = 10;
