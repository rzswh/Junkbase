#pragma once

#include "../index/IndexManager.h"
#include "../recman/RecordManager.h"
#include "../sysman/sysman.h"
#include "../def.h"

/**
 * Used in program to describe column. 
 */ 
struct AttrInfo {
    char * attrName;
    bool notNull;
    AttrType type;
    char * defaultValue;
};

class SystemManager
{
private:
    IndexManager * im;
    RecordManager * rm;
public:
    SystemManager(RecordManager *, IndexManager *);
    ~SystemManager();
    int createTable(const char * tableName, 
        int attrCount, AttrInfo * attributes);
    int dropTable(const char tableName);
    int createIndex(const char * tableName, const char * attrName);
    int dropIndex(const char * tableName, const char * attrName);
    int addPrimaryKey(const char * tableName, const char ** columnList);
    int dropPrimaryKey(const char * tableName);
    int addForeignKey(const char * tableName, 
        const char * foreignKeyName, 
        const char ** columnList);
    int addColumn(const char * tableName, AttrInfo attribute);
    int dropColumn(const char * tableName, char * columnName);
    int changeColumn(const char * tableName, 
        const char * oldColumnName, 
        AttrInfo attribute);
};