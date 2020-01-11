#pragma once

#include "../def.h"
#include "../index/IndexManager.h"
#include "../recman/RecordManager.h"

const char mainTableFilename[] = "_MAIN.db";
const char indexTableFilename[] = "_INDEX.db";
const char PRIMARY_INDEX_NAME[] = "$PRIMARY";

/**
 * Used in program to describe column.
 */
struct AttrInfo {
    char *attrName;
    bool notNull;
    bool isPrimary;
    bool isForeign;
    char *refTableName;
    vector<const char *> refColumnName;
    AttrTypeAtom type;
    int length;
    char *defaultValue;
    vector<const char *> keyCols;
    AttrInfo()
    {
        attrName = refTableName = nullptr;
        defaultValue = nullptr;
    }
    AttrInfo(const char *name, bool notnull, AttrTypeAtom type, int len,
             void *defaultVal = nullptr, char *refTbName = nullptr,
             char *colTbName = nullptr)
        : attrName(const_cast<char *>(name)), notNull(notnull),
          isPrimary(false), isForeign(false), refTableName(refTbName),
          refColumnName(vector<const char *>({colTbName})), type(type),
          length(len), defaultValue((char *)defaultVal)
    {
    }
    AttrInfo(vector<const char *> name)
        : attrName(nullptr), notNull(false), isPrimary(true), isForeign(false),
          refTableName(nullptr), type(TYPE_CHAR), length(0),
          defaultValue(nullptr), keyCols(name)
    {
    }
    AttrInfo(vector<const char *> name, const char *refTbName,
             vector<const char *> refColName)
        : attrName(nullptr), notNull(false), isPrimary(false), isForeign(true),
          refTableName(const_cast<char *>(refTbName)),
          refColumnName(refColName), type(TYPE_CHAR), length(0),
          defaultValue(nullptr), keyCols(name)
    {
    }
};

class SystemManager
{
private:
    bool usingDatabase;
    string currentDatabase;

public:
    SystemManager();
    ~SystemManager();
    int useDatabase(const char *databaseName);
    int showDatabase(const char *databaseName);
    // int createDatabase(const char * databaseName);
    int dropDatabase(const char *databaseName);

    int createTable(const char *tableName, vector<AttrInfo> attributes);
    int dropTable(const char *tableName);
    int createIndex(const char *tableName, const char *keyName,
                    vector<const char *> &attrNames, int indexno,
                    bool isKey = false);
    int dropIndex(const char *tableName, int indexno);
    int addPrimaryKey(const char *tableName, vector<const char *> columnNames);
    int dropPrimaryKey(const char *tableName);
    int addForeignKey(const char *foreignKeyName, const char *tableName,
                      vector<const char *> columnNames,
                      const char *refTableName,
                      vector<const char *> refColumnNames);
    int dropForeignKey(const char *tableName, const char *foreignKeyName);
    int addColumn(const char *tableName, AttrInfo attribute);
    int dropColumn(const char *tableName, char *columnName);
    int changeColumn(const char *tableName, const char *oldColumnName,
                     AttrInfo attribute);
    bool isUsingDatabse() const { return usingDatabase; }

private:
    // int setIndexNo(const char *tableName, const char *columnName, int
    // indexno);
    int allocateIndexNo();
    MRecord findAttrRecord(const char *tableName, const char *columnName);
};

void showDatabases();
