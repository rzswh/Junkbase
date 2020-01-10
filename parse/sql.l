%{
#include <vector>
#include <cstdio>
#include <string>
#include "../sysman/sysman.h"
#include "../def.h"
#include "../utils/type.h"
#include "../parse/parse.h"
#include "../queryman/RelAttr.h"
#include "../queryman/queryman.h"
#include "../queryman/Condition.h"
#include "../queryman/ValueHolder.h"
using std::vector;
using std::string;
#include "sql.tab.hpp"
%}

%option caseless

WHITESPACE   ([ \t\n]+)
IDENTIFIER   ([A-Za-z][_0-9A-Za-z]*)
INTEGER      ([+-]?[0-9]+)
FLOAT        ([+-]?[0-9]+\.[0-9]*|[0-9]*\.[0-9]+)
OPERATOR     ("+"|"-"|"*"|"/"|"%"|"="|"<"|">"|"."|","|";"|"!"|"("|")"|"["|"]"|"{"|"}")
STRING_H     \"([^\\"]|\\.)*\"
%x string

%%

"<="         { return LE; }
">="         { return GE; }
"<>"         { return NE; }
DATABASE	 { return DATABASE; }
DATABASES	 { return DATABASES; }
TABLE	     { return TABLE; }
TABLES	     { return TABLES; }
SHOW	     { return SHOW; }
CREATE       { return CREATE; }
DROP	     { return DROP; }
USE	         { return USE; }
PRIMARY	     { return PRIMARY; }
KEY	         { return KEY; }
NOT	         { return NOT; }
NULL         { return NUL; }
INSERT	     { return INSERT; }
INTO	     { return INTO; }
VALUES	     { return VALUES; }
DELETE	     { return DELETE; }
FROM	     { return FROM; }
WHERE        { return WHERE; }
UPDATE	     { return UPDATE; }
SET	         { return SET; }
SELECT	     { return SELECT; }
IS	         { return IS; }
INT 	     { return TINT; }
CHAR         { return CHAR; }
DATE         { return DATE; }
NUMERIC      { return NUMERIC; }
VARCHAR      { return VARCHAR; }
DEFAULT	     { return DEFAULT; }
CONSTRAINT	 { return CONSTRAINT; }
CHANGE	     { return CHANGE; }
ALTER	     { return ALTER; }
ADD	         { return ADD; }
RENAME       { return RENAME; }
REFERENCES 	 { return REFERENCES; }
INDEX	     { return INDEX; }
AND	         { return AND; }
FOREIGN      { return FOREIGN; }
{STRING_H}   {  
                int n = strlen(yytext + 1);
                yylval.str = new char[n];
                int j = 0; n--;
                for (int i = 0; i < n; i++) {
                    if (yytext[1 + i] == '\\') i++;
                    yylval.str[j++] = yytext[i + 1];
                }
                yylval.str[j++] = '\0';
                return VALUE_STRING;
             }
{WHITESPACE} { ; }
{IDENTIFIER} { 
    int n = strlen(yytext);
    yylval.str = new char[n];
    // yylval.str = yytext; 
    strcpy(yylval.str, yytext);
    // printf("IDENTIFIER:%s\n", yytext); 
    return IDENTIFIER; 
}
{INTEGER}    { yylval.num = atoi(yytext); return VALUE_INT; }
{FLOAT}      { 
    yylval.dec = new Numeric(yytext);
    // yylval.dec->debug();
    return VALUE_FLOAT; 
}
{OPERATOR}   { return *yytext; }
%%

int yywrap() {
    return 1;
}