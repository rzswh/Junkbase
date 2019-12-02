#pragma once

#include "../index/IndexManager.h"
#include "../recman/RecordManager.h"
#include "../sysman/sysman.h"
#include "../def.h"

const char * mainTableFilename = "_MAIN.db";
const char * indexTableFilename = "_INDEX.db";
const char * PRIMARY_INDEX_NAME = "$PRIMARY";

/**
 * Used in program to describe column. 
 */ 
struct AttrInfo {
    char * attrName;
    bool notNull;
    bool isPrimary;
    bool isForeign;
    char * refTableName;
    char * refColumnName;
    AttrType type;
    int length;
    char * defaultValue;
};

class SystemManager
{
private:
    IndexManager * im;
    RecordManager * rm;
    bool usingDatabase;
public:
    SystemManager(RecordManager *, IndexManager *);
    ~SystemManager();
    int useDatabase(const char * databaseName);
    int showDatabase(const char * databaseName);
    // int createDatabase(const char * databaseName);
    // int dropDatabase(const char * databaseName);

    int createTable(const char * tableName, 
        int attrCount, AttrInfo * attributes);
    int dropTable(const char* tableName);
    int createIndex(const char * tableName, 
        const char * keyName, 
        const char * attrName, 
        int indexno);
    int dropIndex(const char * tableName, int indexno);
    int addPrimaryKey(const char * tableName, 
        const char * columnName);
    int dropPrimaryKey(const char * tableName);
    int addForeignKey(const char * tableName, 
        const char * foreignKeyName, 
        const char * columnName);
    int dropForeignKey(const char * tableName, 
        const char * foreignKeyName);
    int addColumn(const char * tableName, AttrInfo attribute);
    int dropColumn(const char * tableName, char * columnName);
    int changeColumn(const char * tableName, 
        const char * oldColumnName, 
        AttrInfo attribute);
private:
    int setIndexNo(const char * tableName, 
        const char * columnName, int indexno);
    MRecord findAttrRecord(const char * tableName, const char * columnName);
};


