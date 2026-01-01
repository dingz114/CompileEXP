%{
#include "ast.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>

extern int yylex();
extern void yyerror(const char* msg);
extern int yylineno;
extern int yycolumn;

// 全局AST根节点
std::unique_ptr<CompUnit> ast_root;
%}

%code requires {
    #include "ast.h"
    #include <memory>
    #include <vector>
    #include <string>
}

/* 错误处理 */
%locations
%define parse.error verbose

/* Token定义 */
%union {
    int num;
    std::string* str;
    CompUnit* comp_unit;
    FuncDef* func_def;
    std::vector<FuncDef*>* func_list;
    Block* block;
    Stmt* stmt;
    std::vector<Stmt*>* stmt_list;
    Expr* expr;
    std::vector<std::unique_ptr<Expr>>* expr_list;
    std::vector<std::string>* param_list;
    FuncDef::ReturnType return_type;
    BinaryExpr::Operator binary_op;
    UnaryExpr::Operator unary_op;
}

/* Token声明 */
%token <num> NUMBER
%token <str> IDENTIFIER
%token INT VOID IF ELSE WHILE BREAK CONTINUE RETURN
%token PLUS MINUS MULTIPLY DIVIDE MODULO
%token LT GT LE GE EQ NE
%token AND OR NOT
%token ASSIGN
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA
%token ERROR_TOKEN

/* 非终结符类型声明 */
%type <comp_unit> CompUnit
%type <func_list> FuncDefList
%type <func_def> FuncDef
%type <return_type> FuncType
%type <param_list> ParamList ParamListOpt
%type <block> Block
%type <stmt_list> StmtList
%type <stmt> Stmt
%type <expr> Expr LOrExpr LAndExpr RelExpr AddExpr MulExpr UnaryExpr PrimaryExpr
%type <expr_list> ArgList ArgListOpt
%type <binary_op> RelOp AddOp MulOp
%type <unary_op> UnaryOp

/* 优先级和结合性 */
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left MULTIPLY DIVIDE MODULO
%right NOT UNARY_MINUS UNARY_PLUS
%left LPAREN RPAREN

/* 开始符号 */
%start CompUnit

%%

/* 编译单元 */
CompUnit: FuncDefList {
    $$ = new CompUnit();
    for (auto func : *$1) {
        $$->functions.push_back(std::unique_ptr<FuncDef>(func));
    }
    delete $1;
    ast_root = std::unique_ptr<CompUnit>($$);
}
;

FuncDefList: FuncDef {
    $$ = new std::vector<FuncDef*>();
    $$->push_back($1);
}
| FuncDefList FuncDef {
    $$ = $1;
    $$->push_back($2);
}
;

/* 函数定义 */
FuncDef: FuncType IDENTIFIER LPAREN ParamListOpt RPAREN Block {
    $$ = new FuncDef($1, *$2, *$4, std::unique_ptr<Block>($6));
    delete $2;
    delete $4;
}
;

FuncType: INT {
    $$ = FuncDef::ReturnType::INT;
}
| VOID {
    $$ = FuncDef::ReturnType::VOID;
}
;

ParamListOpt: /* 空 */ {
    $$ = new std::vector<std::string>();
}
| ParamList {
    $$ = $1;
}
;

ParamList: INT IDENTIFIER {
    $$ = new std::vector<std::string>();
    $$->push_back(*$2);
    delete $2;
}
| ParamList COMMA INT IDENTIFIER {
    $$ = $1;
    $$->push_back(*$4);
    delete $4;
}
;

/* 语句块 */
Block: LBRACE StmtList RBRACE {
    $$ = new Block();
    for (auto stmt : *$2) {
        $$->statements.push_back(std::unique_ptr<Stmt>(stmt));
    }
    delete $2;
}
;

StmtList: /* 空 */ {
    $$ = new std::vector<Stmt*>();
}
| StmtList Stmt {
    $$ = $1;
    $$->push_back($2);
}
;

/* 语句 */
Stmt: Block {
    $$ = $1;
}
| SEMICOLON {
    $$ = new ExprStmt(nullptr);
}
| Expr SEMICOLON {
    $$ = new ExprStmt(std::unique_ptr<Expr>($1));
}
| IDENTIFIER ASSIGN Expr SEMICOLON {
    $$ = new AssignStmt(*$1, std::unique_ptr<Expr>($3));
    delete $1;
}
| INT IDENTIFIER ASSIGN Expr SEMICOLON {
    $$ = new VarDecl(*$2, std::unique_ptr<Expr>($4));
    delete $2;
}
| IF LPAREN Expr RPAREN Stmt {
    $$ = new IfStmt(std::unique_ptr<Expr>($3), std::unique_ptr<Stmt>($5), nullptr);
}
| IF LPAREN Expr RPAREN Stmt ELSE Stmt {
    $$ = new IfStmt(std::unique_ptr<Expr>($3), std::unique_ptr<Stmt>($5), std::unique_ptr<Stmt>($7));
}
| WHILE LPAREN Expr RPAREN Stmt {
    $$ = new WhileStmt(std::unique_ptr<Expr>($3), std::unique_ptr<Stmt>($5));
}
| BREAK SEMICOLON {
    $$ = new BreakStmt();
}
| CONTINUE SEMICOLON {
    $$ = new ContinueStmt();
}
| RETURN SEMICOLON {
    $$ = new ReturnStmt(nullptr);
}
| RETURN Expr SEMICOLON {
    $$ = new ReturnStmt(std::unique_ptr<Expr>($2));
}
;

/* 表达式 */
Expr: LOrExpr {
    $$ = $1;
}
;

LOrExpr: LAndExpr {
    $$ = $1;
}
| LOrExpr OR LAndExpr {
    $$ = new BinaryExpr(BinaryExpr::Operator::OR, std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3));
}
;

LAndExpr: RelExpr {
    $$ = $1;
}
| LAndExpr AND RelExpr {
    $$ = new BinaryExpr(BinaryExpr::Operator::AND, std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3));
}
;

RelExpr: AddExpr {
    $$ = $1;
}
| RelExpr RelOp AddExpr {
    $$ = new BinaryExpr($2, std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3));
}
;

RelOp: LT { $$ = BinaryExpr::Operator::LT; }
| GT { $$ = BinaryExpr::Operator::GT; }
| LE { $$ = BinaryExpr::Operator::LE; }
| GE { $$ = BinaryExpr::Operator::GE; }
| EQ { $$ = BinaryExpr::Operator::EQ; }
| NE { $$ = BinaryExpr::Operator::NE; }
;

AddExpr: MulExpr {
    $$ = $1;
}
| AddExpr AddOp MulExpr {
    $$ = new BinaryExpr($2, std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3));
}
;

AddOp: PLUS { $$ = BinaryExpr::Operator::ADD; }
| MINUS { $$ = BinaryExpr::Operator::SUB; }
;

MulExpr: UnaryExpr {
    $$ = $1;
}
| MulExpr MulOp UnaryExpr {
    $$ = new BinaryExpr($2, std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3));
}
;

MulOp: MULTIPLY { $$ = BinaryExpr::Operator::MUL; }
| DIVIDE { $$ = BinaryExpr::Operator::DIV; }
| MODULO { $$ = BinaryExpr::Operator::MOD; }
;

UnaryExpr: PrimaryExpr {
    $$ = $1;
}
| UnaryOp UnaryExpr %prec UNARY_PLUS {
    $$ = new UnaryExpr($1, std::unique_ptr<Expr>($2));
}
;

UnaryOp: PLUS { $$ = UnaryExpr::Operator::PLUS; }
| MINUS { $$ = UnaryExpr::Operator::MINUS; }
| NOT { $$ = UnaryExpr::Operator::NOT; }
;

PrimaryExpr: IDENTIFIER {
    $$ = new VarExpr(*$1);
    delete $1;
}
| NUMBER {
    $$ = new NumberExpr($1);
}
| LPAREN Expr RPAREN {
    $$ = $2;
}
| IDENTIFIER LPAREN ArgListOpt RPAREN {
    std::vector<std::unique_ptr<Expr>> args;
    for (auto& arg : *$3) {
        args.push_back(std::move(arg));
    }
    $$ = new CallExpr(*$1, std::move(args));
    delete $1;
    delete $3;
}
;

ArgListOpt: /* 空 */ {
    $$ = new std::vector<std::unique_ptr<Expr>>();
}
| ArgList {
    $$ = $1;
}
;

ArgList: Expr {
    $$ = new std::vector<std::unique_ptr<Expr>>();
    $$->push_back(std::unique_ptr<Expr>($1));
}
| ArgList COMMA Expr {
    $$ = $1;
    $$->push_back(std::unique_ptr<Expr>($3));
}
;

%%
