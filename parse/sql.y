%{
#include <vector>
#include <cstdio>
#include <string>
#include "../sysman/sysman.h"
#include "../def.h"
#include "../utils/type.h"
#include "../parse/parse.h"
using std::vector;
using std::string;

SystemManager * sysman;

int yylex(void);
int yyerror(char *msg);

vector<const char *> vectorCharToConst(vector<char*> & arr) {
    vector<const char*> ret;
    for (char * s : arr) ret.push_back(s);
    return ret;
}

%}

%token DATABASE	DATABASES	TABLE	TABLES	SHOW	CREATE
%token DROP	USE	PRIMARY	KEY	NOT	NUL
%token INSERT	INTO	VALUES	DELETE	FROM	WHERE
%token UPDATE	SET	SELECT	IS	TINT	VARCHAR CHAR    NUMERIC
%token DEFAULT	CONSTRAINT	CHANGE	ALTER	ADD	RENAME
%token REFERENCES 	INDEX	AND DATE    FOREIGN
%token LE   GE  NE
%token<str> IDENTIFIER   VALUE_STRING
%token<num> VALUE_INT
%token<dec> VALUE_FLOAT

%left AND

%union {
    int num;
    double dec;
    const char * conststr;
    char * str;
    vector<char *>* strs;
    AttrInfo * attr;
    vector<AttrInfo>* attrs;
    ValueHolder * val;
    vector<ValueHolder> * vals;
    vector<vector<ValueHolder>>* valss;
    AttrTypeComplex * type;
    // int num;
    // vector<AttrInfo> attrs;
    // AttrInfo attr;
    // AttrTypeComplex type;
    // vector<vector<ValueHolder>> valss;
    // vector<ValueHolder> vals;
    // ValueHolder val;
    // vector<const char *> strs;
    // char * str;
}

%type<str> dbName    tbName  colName keyName
%type<attrs> fieldList
%type<attr> fieldMix   field
%type<type> type
%type<valss> valueLists
%type<vals> valueList
%type<val> value
%type<strs> columnList

%%

start:    %empty
        | stmt start
;

stmt:     sysStmt ';' 
        | dbStmt ';' 
        | tbStmt ';' 
		| alterStmt ';'
;

sysStmt:SHOW DATABASES
        {
            showDatabases();
        }
;

dbStmt:   CREATE DATABASE dbName
        {
            string cmd = "create " + string($3);
            if(sysman->isUsingDatabse())
                cmd = string("../") + cmd;
            else 
                cmd = string("./") + cmd;
            system(cmd.c_str());
        }
        | DROP DATABASE dbName
        {
            string cmd = "drop " + string($3);
            if(sysman->isUsingDatabse())
                cmd = string("../") + cmd;
            else 
                cmd = string("./") + cmd;
            system(cmd.c_str());
        }
        | USE dbName
        {
            //
            printf("use dbName=%s\n", $2);
        }
;

tbStmt:   CREATE TABLE tbName '(' fieldList ')'
        {
            #ifdef PRINT_DEBUG
            printf("%s %d\n", $3, $5->size());
            for (AttrInfo info : *$5) printf("-- %s\n", info.attrName);
            #endif
            sysman->createTable($3, *$5);
            delete $5;
        }
        | DROP TABLE tbName
        {
            sysman->dropTable($3);
        }
        | INSERT INTO tbName VALUES valueLists
        {
            // DDL
            printf("insert tableName=%s\n", $3);
            delete $5;
        }
        | DELETE FROM tbName WHERE whereClause
        {
            // DDL
            printf("delete tableName=%s\n", $3);
        }
        | UPDATE tbName SET setClause WHERE whereClause
        {
            // DDL
            printf("update tableName=%s\n", $2);
        }
        | SELECT selector FROM tbName WHERE whereClause
        {
            // DDL
            printf("select tableName=%s\n", $4);
        }
;

alterStmt:ALTER TABLE tbName ADD field
        {
            sysman->addColumn($3, *$5);
            delete $5;
        }
		| ALTER TABLE tbName DROP colName 
        {
            sysman->dropColumn($3, $5);
        }
		| ALTER TABLE tbName CHANGE colName field
        {
            sysman->changeColumn($3, $5, *$6);
            delete $6;
        }
		| ALTER TABLE tbName ADD PRIMARY KEY '(' columnList ')'
        {
            vector<const char*> constColList = vectorCharToConst(*$8);
            sysman->addPrimaryKey($3, constColList);
            for (char * s : *$8) delete [] s;
            delete $8;
        }
		| ALTER TABLE tbName DROP PRIMARY KEY
        {
            sysman->dropPrimaryKey($3);
        }
		| ALTER TABLE tbName ADD CONSTRAINT keyName FOREIGN KEY '(' columnList ')' REFERENCES tbName '(' columnList ')'
        {
            vector<const char*> constColList = vectorCharToConst(*$10);
            vector<const char*> constRefColList = vectorCharToConst(*$10);
            sysman->addForeignKey($3, $6, constColList, $13, constRefColList);
            for (char * s : *$10) delete [] s;
            for (char * s : *$15) delete [] s;
            delete $10, $15;
        }
		| ALTER TABLE tbName DROP FOREIGN KEY keyName
        {
            sysman->dropForeignKey($3, $7);
        }
;

fieldList:fieldMix
        {
            $$ = new vector<AttrInfo>();
            $$->push_back(*$1);
            delete $1;
        }
        | fieldList ',' fieldMix
        {
            $$ = $1;
            $$->push_back(*$3);
            delete $3;
        }
;

field:  colName type
        {
            $$ = new AttrInfo($1, false, $2->type, $2->length);
            delete $2;
        }
        | colName type NOT NUL
        {
            $$ = new AttrInfo($1, true, $2->type, $2->length);
            delete $2;
        }
        | colName type DEFAULT value
        {
            $$ = new AttrInfo($1, false, $2->type, $2->length, $4->buf);
            ($4)->buf = nullptr; // ???
            delete $2, $4;
        }
        | colName type NOT NUL DEFAULT value
        {
            $$ = new AttrInfo($1, true, $2->type, $2->length, $6->buf);
            ($6)->buf = nullptr;
            delete $2, $6;
        }
;

fieldMix: field
        {
            $$ = $1;
        }
        | PRIMARY KEY '(' colName ')'
        {
            $$ = new AttrInfo($4);
        }
        | FOREIGN KEY '(' colName ')' REFERENCES tbName '(' colName ')'
        {
            $$ = new AttrInfo($4, $7, $9);
        }
;

type:     TINT
        {
            $$ = new AttrTypeComplex(TYPE_INT, 4);
        }
        | VARCHAR '(' VALUE_INT ')'
        {
            $$ = new AttrTypeComplex(TYPE_VARCHAR, $3);
        }
        | CHAR '(' VALUE_INT ')'
        {
            $$ = new AttrTypeComplex(TYPE_CHAR, $3);
        }
        | DATE
        {
            $$ = new AttrTypeComplex(TYPE_DATE, sizeof(Date));
        }
        | NUMERIC '(' VALUE_INT ',' VALUE_INT ')'
        {
            $$ = new AttrTypeComplex(TYPE_NUMERIC, $3, $5);
        }
;

valueLists: '(' valueList ')'
        {
            $$ = new vector<vector<ValueHolder>>();
            $$->push_back(*$2);
            delete $2;
        }
        | valueLists ',' '(' valueList ')'
        {
            $$ = $1;
            $$->push_back(*$4);
            delete $4;
        }
;

valueList: value
        {
            $$ = new vector<ValueHolder>();
            $$->push_back(*$1);
            delete $1;
        }
        | valueList ',' value
        {
            $$ = $1;
            $$->push_back(*$3);
            delete $3;
        }
;

value:VALUE_INT
    {
        $$ = new ValueHolder($1);
    }
    | VALUE_STRING
    {
        $$ = new ValueHolder($1);
    }
    | VALUE_FLOAT
    {
        $$ = new ValueHolder($1);
    }
    | NUL
    {
        $$ = new ValueHolder();
    }
;

whereClause:  col op expr
            | col IS NUL
            | col IS NOT NUL
            | whereClause AND whereClause
;

col:      tbName '.' colName
        | colName
;

op:   '=' 
    | NE
    | LE
    | GE
    | '<' 
    | '>'
;

expr: value
    | col
;

setClause:colName '=' value
        | setClause ',' colName '=' value
;

selector: '*' 
        | columnList
;

columnList: colName
            {
                $$ = new vector<char *>();
                $$->push_back($1);
            }
            | columnList ',' colName
            {
                $$ = $1;
                $$->push_back($3);
            }
;

dbName: IDENTIFIER {
    // int N = strlen($1);
    // $$ = new char[N];
    // strcpy($$, $1);
    $$ = $1; 
};
tbName: IDENTIFIER { 
    $$ = $1; 
    // printf("tbName : %s\n", $$);
};
colName: IDENTIFIER { 
    $$ = $1; 
};
keyName: IDENTIFIER { 
    $$ = $1; 
};

%%

int main() {
    sysman = new SystemManager();
    return yyparse();
}
int yyerror(char *msg)
{
    printf("Statement parsing error: %s\n", msg);
}