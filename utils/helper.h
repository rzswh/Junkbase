#pragma once

#include "../def.h"
#include "../recman/RID.h"
#include <cstdint>
#include <string>
using std::string;

class RecordHelper
{
public:
    static int getOffset(void *d_ptr);
    static int getLength(void *d_ptr);
    static AttrTypeAtom getType(void *d_ptr);
    static bool getNotNull(void *d_ptr);
    static int getIndex(void *d_ptr);
    static RID getDefaultValueRID(void *d_ptr);
    static RID getRefTableNameRID(void *d_ptr);
    static RID getRefColumnNameRID(void *d_ptr);
    static string getAttrName(void *d_ptr);
};
class IndexHelper
{
public:
    static string getIndexName(void *d_ptr);
    static int getIndexNo(void *d_ptr);
    static string getAttrName(void *d_ptr);
    static int getRank(void *d_ptr);
    static bool getIsKey(void *d_ptr);
};