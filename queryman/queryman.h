#pragma once

#include "../parse/parse.h"
#include "Condition.h"
#include <vector>
using std::vector;

class QueryManager
{
public:
    QueryManager();
    ~QueryManager();
    int insert(const char *tableName, vector<ValueHolder> vals);
    int insert(const char *tableName, vector<vector<ValueHolder>> vals);
    int select(vector<RelAttr> &selAttrs, vector<char *> &tableNames,
               Condition &condition);

    int deletes(const char *tableName, Condition &conditions);

    int update(const char *tableName, vector<SetClause> &updSet,
               Condition &condition);
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