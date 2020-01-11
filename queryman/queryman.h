#pragma once

#include "../parse/parse.h"
#include "../recman/FileHandle.h"
#include "Condition.h"
#include "indexOps.h"
#include <vector>
using std::vector;

class QueryManager
{
public:
    QueryManager();
    ~QueryManager();
    int insert(const char *tableName, vector<ValueHolder> vals, FileHandle *fht,
               FileHandle *fhm, IndexPreprocessingData *prep);
    int insert(const char *tableName, vector<vector<ValueHolder>> vals);
    int select(vector<RelAttr> &selAttrs, vector<char *> &tableNames,
               Condition &condition);

    int deletes(const char *tableName, Condition &conditions);

    int update(const char *tableName, vector<SetClause> &updSet,
               Condition &condition);

    int fileImport(const char *fileName, const char *format, char delimiter,
                   const char *tableName);

    static int getAttrStorage(const char *tableName,
                              vector<const char *> attrNames, int *offsets,
                              int *lengths, AttrTypeAtom *types);
};

class PrintableTable
{
    vector<string> headers;
    vector<ValueHolder *> vals;
    ValueHolder *row;

public:
    PrintableTable(const vector<string> &headers);
    ~PrintableTable();
    void addRow();
    void setData(int index, const ValueHolder &v);
    void show();
};