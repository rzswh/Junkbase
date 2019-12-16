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
    AttrInfo() { 
        attrName = refTableName = refColumnName = nullptr; 
        defaultValue = nullptr; 
    }
    AttrInfo(
        const char * name, 
        bool notnull,
        AttrTypeAtom type, 
        int len,
        void * defaultVal = nullptr, 
        char * refTbName = nullptr,
        char * colTbName = nullptr
    ) : attrName(const_cast<char*> (name)), notNull(notnull), 
        isPrimary(false), isForeign(false),
        refTableName(refTbName), refColumnName(colTbName), type(type),
        length(len), defaultValue((char*)defaultVal)
    {}
    AttrInfo(const char * name) : attrName(const_cast<char*> (name)), 
        notNull(false), isPrimary(true),
        isForeign(false), refTableName(nullptr), refColumnName(nullptr),
        type(TYPE_CHAR), length(0), defaultValue(nullptr) 
    {}
    AttrInfo(const char * name, const char * refTbName, const char * colTbName)
    : attrName(const_cast<char*> (name)), notNull(false), isPrimary(false),
        isForeign(true), refTableName(const_cast<char*> (refTbName)), 
        refColumnName(const_cast<char*> (colTbName)),
        type(TYPE_CHAR), length(0), defaultValue(nullptr) 
    {}
};

class SystemManager
{
private:
    bool usingDatabase;
public:
    SystemManager();
    ~SystemManager();
    int useDatabase(const char * databaseName);
    int showDatabase(const char * databaseName);
    // int createDatabase(const char * databaseName);
    // int dropDatabase(const char * databaseName);

    int createTable(const char * tableName, 
        vector<AttrInfo> attributes);
    int dropTable(const char* tableName);
    int createIndex(const char * tableName, 
        const char * keyName, 
        vector<const char *>& attrNames, 
        int indexno);
    int dropIndex(const char * tableName, int indexno);
    int addPrimaryKey(const char * tableName, 
        vector<const char *> columnNames);
    int dropPrimaryKey(const char * tableName);
    int addForeignKey(const char * foreignKeyName, 
        const char * tableName, 
        vector<const char *> columnNames,
        const char * refTableName,
        vector<const char*> refColumnNames);
    int dropForeignKey(const char * tableName, 
        const char * foreignKeyName);
    int addColumn(const char * tableName, AttrInfo attribute);
    int dropColumn(const char * tableName, char * columnName);
    int changeColumn(const char * tableName, 
        const char * oldColumnName, 
        AttrInfo attribute);
    bool isUsingDatabse() const { return usingDatabase; }
private:
    int setIndexNo(const char * tableName, 
        const char * columnName, int indexno);
    int allocateIndexNo();
    MRecord findAttrRecord(const char * tableName, const char * columnName);
};

void showDatabases();

