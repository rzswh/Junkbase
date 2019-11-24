#pragma once

enum AttrType {
    TYPE_CHAR    = 1, 
    TYPE_VARCHAR = 2, 
    TYPE_NUMERIC = 3, 
    TYPE_INT     = 4, 
    TYPE_DATE    = 5, 
    // internal type
    TYPE_CHR_RID = 9,
    TYPE_VCH_RID = 10,
    TYPE_NMR_RID = 11,
    TYPE_INT_RID = 12,
    TYPE_DAT_RID = 13
};

// const int MAX_ATTR_NAME_LEN = ;
