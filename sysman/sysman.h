#pragma once

#include "../index/IndexManager.h"
#include "../recman/RecordManager.h"
#include "../def.h"

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
    AttrTypeAtom type;
    int length;
    char * defaultValue;
    AttrInfo() { attrName = refTableName = refColumnName = nullptr; defaultValue = nullptr; }
    AttrInfo(
        char * name, 
        bool notnull,
        bool primary, 
        bool foreign,
        AttrTypeAtom type, 
        int len,
        void * defaultVal = nullptr, 
        char * refTbName = nullptr,
        char * colTbName = nullptr
    ) : attrName(name), notNull(notnull), isPrimary(primary), isForeign(foreign),
        refTableName(refTbName), refColumnName(colTbName), type(type),
        length(len), defaultValue((char*)defaultVal)
    {}
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
        vector<const char *>& attrNames, 
        int indexno);
    int dropIndex(const char * tableName, int indexno);
    int addPrimaryKey(const char * tableName, 
        vector<const char *> columnNames);
    int dropPrimaryKey(const char * tableName);
    int addForeignKey(const char * tableName, 
        const char * foreignKeyName, 
        vector<const char *> columnNames);
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
    int allocateIndexNo();
    MRecord findAttrRecord(const char * tableName, const char * columnName);
};


