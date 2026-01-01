%{
#include "parser.hpp"
#include <string>
#include <cstdlib>

// 行号和列号跟踪
int yylineno = 1;
int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

// 更新行号
void update_line() {
    yylineno++;
    yycolumn = 1;
}
%}

%option noyywrap
%option yylineno

/* 正则表达式定义 */
DIGIT       [0-9]
LETTER      [a-zA-Z_]
ALNUM       [a-zA-Z0-9_]
NUMBER      -?(0|[1-9]{DIGIT}*)
IDENTIFIER  {LETTER}{ALNUM}*
WHITESPACE  [ \t]+
NEWLINE     \r?\n

/* 单行注释 */
SINGLE_LINE_COMMENT "//"[^\r\n]*

/* 多行注释状态 */
%x COMMENT

%%

{WHITESPACE}    { /* 忽略空白字符 */ }
{NEWLINE}       { update_line(); }

 /* 单行注释 */
{SINGLE_LINE_COMMENT}   { /* 忽略单行注释 */ }

 /* 多行注释开始 */
"/*"            { BEGIN(COMMENT); }
<COMMENT>"*/"   { BEGIN(INITIAL); }
<COMMENT>{NEWLINE} { update_line(); }
<COMMENT>.      { /* 忽略注释内容 */ }

 /* 关键字 */
"int"           { return INT; }
"void"          { return VOID; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }
"return"        { return RETURN; }

 /* 运算符 */
"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MULTIPLY; }
"/"             { return DIVIDE; }
"%"             { return MODULO; }

 /* 关系运算符 */
"<"             { return LT; }
">"             { return GT; }
"<="            { return LE; }
">="            { return GE; }
"=="            { return EQ; }
"!="            { return NE; }

 /* 逻辑运算符 */
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }

 /* 赋值运算符 */
"="             { return ASSIGN; }

 /* 分隔符 */
"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
";"             { return SEMICOLON; }
","             { return COMMA; }

 /* 标识符 */
{IDENTIFIER}    { 
    yylval.str = new std::string(yytext); 
    return IDENTIFIER; 
}

 /* 数字 */
{NUMBER}        { 
    yylval.num = atoi(yytext); 
    return NUMBER; 
}

 /* 未识别字符 */
.               { 
    fprintf(stderr, "Lexical error: Unexpected character '%c' at line %d, column %d\n", 
            yytext[0], yylineno, yycolumn); 
    return ERROR_TOKEN;
}

%%

/* 用户代码区域 */
void yyerror(const char* msg) {
    fprintf(stderr, "Parse error: %s at line %d, column %d\n", msg, yylineno, yycolumn);
}
