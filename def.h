#pragma once

// #define IF_RET_ERROR(S, R) if ((R = (S)) == 0)

enum AttrTypeAtom {
    TYPE_CHAR    = 1, 
    TYPE_VARCHAR = 2, 
    TYPE_NUMERIC = 3, 
    TYPE_INT     = 4, 
    TYPE_DATE    = 5, 
    TYPE_RID     = 6,
    // internal type
    // TYPE_CHR_RID = 9,
    // TYPE_VCH_RID = 10,
    // TYPE_NMR_RID = 11,
    // TYPE_INT_RID = 12,
    // TYPE_DAT_RID = 13
};

class AttrTypeHelper {
    static int lowestKey(AttrTypeAtom target, AttrType pattern) {
        AttrTypeAtom ret = target;
        while (pattern) pattern >>= 3, target <<= 3;
        return ret;
    }
    static int countKey(AttrType type) {
        int ret = 0;
        for (; type; type >>= 3) ret ++;
        return ret;
    }
};

// compound type
// lower bit has higher priority
typedef unsigned int AttrType;

const int VARIANT_SEGMENT_LENGTH = 64;
const int MAX_TABLE_NAME_LEN = 30;
const int MAX_ATTR_NAME_LEN = 20;
const int MAX_KEY_NAME_LEN = 20;
