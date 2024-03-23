%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"

#define YYDEBUG 1
// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart();
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
syntax_tree_node *node(const char *node_name,int children_num, ...);
%}


%union {
    struct _syntax_tree_node *node;
}


%token<node> ERROR 
%token<node> ADD SUB MUL DIV MOD
%token<node> LT GT LE GE EQ NEQ
%token<node> ASSIGN SEMICOLON COMMA LPARENTHESIS RPARENTHESIS LBRACE RBRACE LBRACKET RBRACKET
%token<node> INT FLOAT VOID CONST
%token<node> IF ELSE WHILE CONTINUE BREAK RETURN
%token<node> AND OR NOT
%token<node> ID INTEGER_10 INTEGER_8 INTEGER_16 FLOATPOINT_10 FLOATPOINT_16 

%type<node> CompUnit Decl ConstDecl ConstDefList ConstDef ConstInitVal VarDecl VarDefList VarDef InitVal FuncDef FuncFParams FuncFParam Block BlockItem
%type<node> Stmt Exp Cond LVal PrimaryExp Number UnaryExp FuncRParams MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type<node> UnaryOp
%type<node> BType FuncType
%type<node> ConstArrayIdent
%type<node> ConstInitValArrayList  
%type<node> InitValArrayList
%type<node> FuncArrayIdent        
%type<node> BlockItemList          
%type<node> ArrayIdent             
%type<node> INTEGER FLOATPOINT
%type<node> empty

%start CompUnit
%left ASSIGN
%left ADD SUB
%left MUL DIV MOD
%left OR
%left AND
%left NOT

%%

CompUnit : Decl { $$ = node("CompUnit", 1, $1);gt->root = $$;}
         | CompUnit Decl { $$ = node("CompUnit",2, $1, $2);gt->root = $$;}
         ;

Decl : FuncDef { $$ = node("Decl", 1, $1); }
        | ConstDecl { $$ = node("Decl", 1, $1); }
        | VarDecl { $$ = node("Decl", 1, $1); }
        ;

ConstDecl : CONST BType ConstDefList SEMICOLON { $$ = node("ConstDecl", 4,$1, $2, $3, $4); }
        ;

ConstDefList : ConstDefList COMMA ConstDef { $$ = node("ConstDef", 3, $1, $2, $3); }
        | ConstDef { $$ = node("ConstDef", 1, $1); }
        ;

BType : INT { $$ = node("BType", 1, $1); }
        | FLOAT { $$ = node("BType", 1, $1); }
        ;

ConstDef : ID ASSIGN ConstInitVal { $$ = node("ConstDef", 3, $1, $2, $3); }
        |  ID ConstArrayIdent ASSIGN ConstInitVal { $$ = node("ConstDef", 4, $1, $2, $3,$4);}
        ;

ConstInitVal : ConstExp { $$ = node("ConstInitVal", 1, $1); }
        | LBRACE ConstInitValArrayList RBRACE { $$ = node("ConstInitVal", 3,$1, $2, $3); }
        | LBRACE RBRACE { $$ = node("ConstInitVal", 2, $1, $2); }
        ;

ConstInitValArrayList : ConstInitValArrayList COMMA ConstInitVal { $$ = node("ConstInitValArrayList", 3, $1, $2, $3); }
        | ConstInitVal { $$ = node("ConstInitValArrayList", 1, $1); }
        ;

VarDecl : BType VarDefList SEMICOLON { $$ = node("VarDecl", 3, $1, $2, $3); }
        ;

VarDefList : VarDefList COMMA VarDef { $$ = node("VarDefList", 3, $1, $2, $3); }
        | VarDef { $$ = node("VarDefList", 1, $1); }
        ;

VarDef : ID { $$ = node("VarDef", 1, $1); }
        | ID ConstArrayIdent { $$ = node("VarDef", 2, $1,$2); }
        | ID ASSIGN Exp { $$ = node("VarDef", 3, $1, $2, $3); }
        | ID ConstArrayIdent ASSIGN InitVal { $$ = node("VarDef", 4, $1, $2, $3, $4); }
        ;

ConstArrayIdent : LBRACKET ConstExp RBRACKET { $$ = node("ConstArrayIdent", 3,$1,$2,$3); }
        | ConstArrayIdent LBRACKET ConstExp RBRACKET { $$ = node("ConstArrayIdent", 4, $1, $2, $3, $4); }
        ;
InitVal :  Exp { $$ = node("InitVal", 1, $1); }
        | LBRACE InitValArrayList RBRACE { $$ = node("InitVal", 3, $1, $2, $3); }
        | LBRACE RBRACE { $$ = node("InitVal", 2, $1, $2); }
        ;

InitValArrayList : InitValArrayList COMMA InitVal { $$ = node("InitValArrayList", 3, $1, $2, $3); }
        | InitVal { $$ = node("InitValArrayList",1,$1);}
        ;

FuncDef :   BType ID LPARENTHESIS FuncFParams RPARENTHESIS Block { $$ = node("FuncDef", 6, $1, $2, $3, $4, $5, $6); }
        |   BType ID LPARENTHESIS RPARENTHESIS Block { $$ = node("FuncDef", 5, $1, $2, $3, $4, $5); }
        |   VOID ID LPARENTHESIS FuncFParams RPARENTHESIS Block { $$ = node("FuncDef", 6, $1, $2, $3, $4, $5, $6); }
        |   VOID ID LPARENTHESIS RPARENTHESIS Block { $$ = node("FuncDef", 5, $1, $2, $3, $4, $5); }
        ;

//FuncType :  BType{ $$ = node("FuncType", 1, $1); }
  //      |   VOID { $$ = node("FuncType", 1, $1); }
    //    ;

FuncFParams :   FuncFParams COMMA FuncFParam { $$ = node("FuncFParams", 3, $1, $2, $3); }
        |   FuncFParam { $$ = node("FuncFParams", 1, $1); }
        ;

FuncFParam :   BType ID { $$ = node("FuncFParam", 2, $1, $2); }
        |   BType ID LBRACKET RBRACKET FuncArrayIdent { $$ = node("FuncFParam", 5, $1, $2,$3,$4,$5); }
        ;

FuncArrayIdent :  FuncArrayIdent LBRACKET Exp RBRACKET { $$ = node("FuncArrayIdent", 4, $1, $2, $3, $4); } 
        |   empty { $$ = node("FuncArrayIdent", 0); }
        ;

Block : LBRACE BlockItemList RBRACE { $$ = node("Block", 3, $1, $2, $3); }
        | LBRACE RBRACE { $$ = node("Block", 2, $1, $2); }
        ;

BlockItemList : BlockItemList BlockItem { $$ = node("BlockItemList", 2, $1, $2); }
        | BlockItem { $$ = node("BlockItemList", 1, $1); }
        ;

BlockItem : Stmt { $$ = node("BlockItem", 1, $1); }
        | ConstDecl { $$ = node("BlockItem", 1, $1); }
        | VarDecl { $$ = node("BlockItem", 1, $1); }
        ;

Stmt : LVal ASSIGN Exp SEMICOLON { $$ = node("Stmt", 4, $1, $2, $3, $4); }
        | SEMICOLON { $$ = node("Stmt", 1, $1);}
        | Exp SEMICOLON { $$ = node("Stmt", 2, $1, $2); }
        | Block { $$ = node("Stmt", 1, $1); }
        | IF LPARENTHESIS Cond RPARENTHESIS Stmt { $$ = node("Stmt", 5, $1, $2, $3, $4, $5); }
        | IF LPARENTHESIS Cond RPARENTHESIS Stmt ELSE Stmt { $$ = node("Stmt", 7, $1, $2, $3, $4, $5, $6, $7); }
        | WHILE LPARENTHESIS Cond RPARENTHESIS Stmt { $$ = node("Stmt", 5, $1, $2, $3, $4, $5); }
        | RETURN SEMICOLON { $$ = node("Stmt", 2, $1, $2); }
        | RETURN Exp SEMICOLON { $$ = node("Stmt", 3, $1, $2, $3); }
        | BREAK SEMICOLON { $$ = node("Stmt", 2, $1, $2); }
        | CONTINUE SEMICOLON { $$ = node("Stmt", 2, $1, $2); }
        ;

Exp : AddExp { $$ = node("Exp", 1, $1); }
        ;

Cond : LOrExp { $$ = node("Cond", 1, $1); }
        ;

LVal : ID { $$ = node("LVal", 1, $1); }
        |  ID ArrayIdent { $$ = node("LVal", 2, $1, $2); }
        ;

ArrayIdent: ArrayIdent LBRACKET Exp RBRACKET { $$ = node("ArrayIdent", 4, $1, $2, $3, $4); }
        | LBRACKET Exp RBRACKET { $$ = node("ArrayIdent", 3, $1, $2, $3); }
        ;

PrimaryExp : LPARENTHESIS Exp RPARENTHESIS { $$ = node("PrimaryExp", 3, $1, $2, $3); }
        | LVal { $$ = node("PrimaryExp", 1, $1); }
        | Number { $$ = node("PrimaryExp", 1, $1); }
        ;
Number :  INTEGER { $$ = node("Number", 1, $1); }
        | FLOATPOINT { $$ = node("Number", 1, $1); }
        ;

INTEGER :  INTEGER_10 { $$ = node("Integer", 1, $1); }
        | INTEGER_8 { $$ = node("Integer", 1, $1); }
        | INTEGER_16 { $$ = node("Integer", 1, $1); }
        ;
FLOATPOINT: FLOATPOINT_10 { $$ = node("FloatPoint", 1, $1); }
        | FLOATPOINT_16 { $$ = node("FloatPoint", 1, $1); }
        ;

UnaryExp : PrimaryExp { $$ = node("UnaryExp", 1, $1); }
        | UnaryOp UnaryExp { $$ = node("UnaryExp", 2, $1, $2); }
        | ID LPARENTHESIS FuncRParams RPARENTHESIS { $$ = node("UnaryExp", 4, $1, $2, $3, $4); }
        | ID LPARENTHESIS RPARENTHESIS { $$ = node("UnaryExp", 3, $1, $2, $3); }
        ;

UnaryOp : ADD { $$ = node("UnaryOp", 1, $1); }
        | SUB { $$ = node("UnaryOp", 1, $1); }
        | NOT { $$ = node("UnaryOp", 1, $1); }
        ;

FuncRParams : FuncRParams COMMA Exp { $$ = node("FuncRParams", 3, $1, $2, $3); }
        | Exp { $$ = node("FuncRParams", 1, $1); }
        ;

MulExp : UnaryExp { $$ = node("MulExp", 1, $1); }
        | MulExp MUL UnaryExp { $$ = node("MulExp", 3, $1, $2, $3); }
        | MulExp DIV UnaryExp { $$ = node("MulExp", 3, $1, $2, $3); }
        | MulExp MOD UnaryExp { $$ = node("MulExp", 3, $1, $2, $3); }
        ;
AddExp : MulExp { $$ = node("AddExp", 1, $1); }
        | AddExp ADD MulExp { $$ = node("AddExp", 3, $1, $2, $3); }
        | AddExp SUB MulExp { $$ = node("AddExp", 3, $1, $2, $3); }
        ;

RelExp : AddExp { $$ = node("RelExp", 1, $1); }
        | RelExp LT AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        | RelExp GT AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        | RelExp LE AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        | RelExp GE AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        ;

EqExp : RelExp { $$ = node("EqExp", 1, $1); }
        | EqExp EQ RelExp { $$ = node("EqExp", 3, $1, $2, $3); }
        | EqExp NEQ RelExp { $$ = node("EqExp", 3, $1, $2, $3); }
        ;

LAndExp : EqExp { $$ = node("LAndExp", 1, $1); }
        | LAndExp AND EqExp { $$ = node("LAndExp", 3, $1, $2, $3); }
        ;

LOrExp : LAndExp { $$ = node("LOrExp", 1, $1); }
        | LOrExp OR LAndExp { $$ = node("LOrExp",3, $1, $2, $3); }
        ;

ConstExp : AddExp { $$ = node("ConstExp", 1, $1); }
        ;

empty : {}
;
%%

/// The error reporting function.
void yyerror(const char * s)
{
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
syntax_tree *parse(const char *input_path)
{

    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

   // std::cout << "Parsing..." << std::endl;
    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

/// A helper function to quickly construct a tree node.
///
/// e.g. $$ = node("program", 1, $1);
syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name,lines);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon",lines);
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
