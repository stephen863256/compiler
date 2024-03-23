#pragma once

extern "C" {
#include "syntax_tree.h"
extern syntax_tree *parse(const char *input);
}
#include "User.hpp"
#include <memory>
#include <string>
#include <vector>

enum SysYType { TYPE_INT, TYPE_FLOAT, TYPE_VOID };

enum RelOp {
    // <=
    OP_LE,
    // <
    OP_LT,
    // >
    OP_GT,
    // >=
    OP_GE,
    // ==
    OP_EQ,
    // !=
    OP_NEQ
};

enum AddOp {
    // +
    OP_PLUS,
    // -
    OP_MINUS
};

enum MulOp {
    // *
    OP_MUL,
    // /
    OP_DIV,
    // %
    OP_MOD
};

enum UnaryOp {
    // +
    OP_POS,
    // -
    OP_NEG,
    // !
    OP_NOT
};

enum LogOp{
    // &&
    OP_AND,
    // ||
    OP_OR
};

class AST;


struct ASTNode;
struct ASTCompUnit;
struct ASTDecl;
struct ASTConstDecl;
//struct ASTConstDefList;
struct ASTConstDef;
struct ASTConstInitVal;
struct ASTVarDecl;
struct ASTVarDef;
//struct ASTVarDefList;
struct ASTInitVal;
struct ASTFuncDef;
struct ASTFuncFParams;
struct ASTFuncFParam;
struct ASTBlock;
struct ASTBlockItem;
struct ASTStmt;
struct ASTExp;
struct ASTCond;
struct ASTLVal;
struct ASTPrimaryExp;
struct ASTNumber;
struct ASTUnaryExp;
//struct ASTFuncRParams;
struct ASTMulExp;
struct ASTAddExp;
struct ASTRelExp;
struct ASTEqExp;
struct ASTLAndExp;
struct ASTLOrExp;
struct ASTConstExp;
struct ASTUnaryOp;
//struct ASTBType;
struct ASTFuncType;
struct ASTConstArrayIdent; 
//struct ASTConstInitValArrayList;
struct ASTInitValArrayList;
struct ASTFuncArrayIdent;   
//struct ASTBlockItemList;     
struct ASTArrayIdent;      
struct ASTAssignStmt;
struct ASTSelectionStmt;
struct ASTIterationStmt;
struct ASTReturnStmt;
struct ASTBreakStmt;
struct ASTContinueStmt;
struct ASTCall;
/*
struct ASTNode;
struct ASTProgram;
struct ASTDeclaration;
struct ASTNum;
struct ASTVarDeclaration;
struct ASTFunDeclaration;
struct ASTParam;
struct ASTCompoundStmt;
struct ASTStatement;
struct ASTExpressionStmt;
struct ASTSelectionStmt;
struct ASTIterationStmt;
struct ASTReturnStmt;
struct ASTFactor;
struct ASTExpression;
struct ASTVar;
struct ASTAssignExpression;
struct ASTSimpleExpression;
struct ASTAdditiveExpression;
struct ASTTerm;
struct ASTCall;
*/
class ASTVisitor;

class AST {
  public:
    AST() = delete;
    AST(syntax_tree *);
    AST(AST &&tree) {
        root = tree.root;
        tree.root = nullptr;
    };
    ASTCompUnit *get_root() { return root.get(); }
    void run_visitor(ASTVisitor &visitor);

  private:
    ASTNode *transform_node_iter(syntax_tree_node *);
    //std::shared_ptr<ASTProgram> root = nullptr;
    std::shared_ptr<ASTCompUnit> root = nullptr;
};

struct ASTNode {
    int lineno = 0;
    virtual Value *accept(ASTVisitor &) = 0;
    virtual ~ASTNode() = default;
};

/*struct ASTProgram : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTProgram() = default;
    std::vector<std::shared_ptr<ASTDeclaration>> declarations;
};*/

struct ASTCompUnit : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTCompUnit() = default;
    std::vector<std::shared_ptr<ASTDecl>> declarations;
    //std::vector<std::shared_ptr<ASTFuncDef>> func_defs;
};

/*struct ASTDeclaration : ASTNode {
    virtual ~ASTDeclaration() = default;
    SysYType type;
    std::string id;
};*/

struct ASTDecl : ASTNode {
    virtual ~ASTDecl() = default;
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTConstDecl> const_decl;
    std::shared_ptr<ASTVarDecl> var_decl;
    std::shared_ptr<ASTFuncDef> func_def;
};

struct ASTConstDecl : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstDecl() = default;
    SysYType type;
    //std::shared_ptr<ASTConstDefList> const_def_list;
    std::vector<std::shared_ptr<ASTConstDef>> const_defs;
};

/*struct ASTConstDefList : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstDefList() = default;
    std::vector<std::shared_ptr<ASTConstDef>> const_defs;
};*/

struct ASTConstDef : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstDef() = default;
//    SysYType type;
    std::string id;
    std::shared_ptr<ASTConstArrayIdent> const_array_ident;
    std::shared_ptr<ASTConstInitVal> const_init_val;
};

struct ASTConstInitVal : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstInitVal() = default;
   // std::vector<int> array_size;//需要数组的大小
    std::shared_ptr<ASTConstExp> const_exp;
    //std::shared_ptr<ASTConstInitValArrayList> const_init_val_array_list;
    std::vector<std::shared_ptr<ASTConstInitVal>> const_init_vals;
};

struct ASTConstArrayIdent : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstArrayIdent() = default;
    std::vector<std::shared_ptr<ASTConstExp>> const_exps;
    //std::string id;
};

struct ASTConstInitValArrayList : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstInitValArrayList() = default;
    std::vector<std::shared_ptr<ASTConstInitVal>> const_init_vals;
};

struct ASTVarDecl : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTVarDecl() = default;
    SysYType type;
    //std::shared_ptr<ASTVarDefList> var_defs;
    std::vector<std::shared_ptr<ASTVarDef>> var_defs;
};

/*struct ASTVarDefList : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTVarDefList() = default;
    std::vector<std::shared_ptr<ASTVarDef>> var_defs;
};*/

struct ASTVarDef : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTVarDef() = default;
   // SysYType type;
    std::string id;
    std::shared_ptr<ASTConstArrayIdent> array_ident;
    std::shared_ptr<ASTInitVal> init_val;
};

struct ASTInitVal : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTInitVal() = default;
    std::shared_ptr<ASTExp> exp;
    std::vector<std::shared_ptr<ASTInitVal>> init_vals;
};

/*struct ASTInitValArrayList : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTInitValArrayList() = default;
    std::vector<std::shared_ptr<ASTInitVal>> init_vals;
};*/

struct ASTFuncDef : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTFuncDef() = default;
    SysYType type;
    std::string id;
    std::vector<std::shared_ptr<ASTFuncFParam>> func_fparams;
    std::shared_ptr<ASTBlock> block;
};

/*struct ASTFuncFParams : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTFuncFParams() = default;
    std::vector<std::shared_ptr<ASTFuncFParam>> func_fparams;
};*/

struct ASTFuncFParam : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTFuncFParam() = default;
    SysYType type;
    bool isarray;
    std::string id;
    std::shared_ptr<ASTFuncArrayIdent> array_ident;
};

struct ASTFuncArrayIdent : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTFuncArrayIdent() = default;
    std::vector<std::shared_ptr<ASTExp>> exps;
    //std::string id;
};

struct ASTBlock : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTBlock() = default;
    std::vector<std::shared_ptr<ASTBlockItem>> block_items;
};

struct ASTBlockItem : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTBlockItem() = default;
    std::shared_ptr<ASTStmt> stmt;
    std::shared_ptr<ASTConstDecl> const_decl;
    std::shared_ptr<ASTVarDecl> var_decl;
};

struct ASTStmt : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTStmt() = default;
    std::shared_ptr<ASTBreakStmt> break_stmt;
    std::shared_ptr<ASTContinueStmt> continue_stmt;
    std::shared_ptr<ASTReturnStmt> return_stmt;
    std::shared_ptr<ASTIterationStmt> iteration_stmt;
    std::shared_ptr<ASTSelectionStmt> selection_stmt;
    std::shared_ptr<ASTAssignStmt> assign_stmt;
    std::shared_ptr<ASTExp> exp;
    std::shared_ptr<ASTBlock> block;
};

struct ASTExp : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTExp() = default;
    std::shared_ptr<ASTAddExp> add_exp;
}
;
struct ASTCond : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTCond() = default;
    std::shared_ptr<ASTLOrExp> l_or_exp;
}
;
struct ASTLVal : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTLVal() = default;
    std::string id;
    std::shared_ptr<ASTArrayIdent> array_ident;
}
;
struct ASTBreakStmt : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTBreakStmt() = default;
}
;
struct ASTContinueStmt : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTContinueStmt() = default;
}
;
struct ASTReturnStmt : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTReturnStmt() = default;
    std::shared_ptr<ASTExp> exp;
}
;
struct ASTAssignStmt : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTAssignStmt() = default;
    std::shared_ptr<ASTLVal> l_val;
    std::shared_ptr<ASTExp> exp;
}
;
struct ASTSelectionStmt : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTSelectionStmt() = default;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> if_stmt;
    std::shared_ptr<ASTStmt> else_stmt;
}
;
struct ASTIterationStmt : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTIterationStmt() = default;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> stmt;
}
;
struct ASTArrayIdent : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTArrayIdent() = default;
    std::vector<std::shared_ptr<ASTExp>> exps;
    //std::string id;
}
;
struct ASTPrimaryExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTPrimaryExp() = default;
    std::shared_ptr<ASTLVal> l_val;
    std::shared_ptr<ASTNumber> number;
    std::shared_ptr<ASTExp> exp;
}
;
struct ASTNumber : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTNumber() = default;
    SysYType type;
    int int_val = 0;
    float float_val = 0;
}
;
struct ASTUnaryExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTUnaryExp() = default;
    UnaryOp  op;
    std::shared_ptr<ASTPrimaryExp> primary_exp;
    std::shared_ptr<ASTUnaryExp> unary_exp;
    std::shared_ptr<ASTCall> call;
}
;
struct ASTCall : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTCall() = default;
    std::string id;
    //std::shared_ptr<ASTFuncRParams> func_rparams;
    std::vector<std::shared_ptr<ASTExp>> exps;
}
;
struct ASTMulExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTMulExp() = default;
    std::shared_ptr<ASTUnaryExp> unary_exp;
    std::shared_ptr<ASTMulExp> mul_exp;
    MulOp op;
};

struct ASTAddExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTAddExp() = default;
    std::shared_ptr<ASTMulExp> mul_exp;
    std::shared_ptr<ASTAddExp> add_exp;
    AddOp op;
}
;
struct ASTRelExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTRelExp() = default;
    std::shared_ptr<ASTRelExp> rel_exp;
    std::shared_ptr<ASTAddExp> add_exp;
    RelOp op;
}
;
struct ASTEqExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTEqExp() = default;
    std::shared_ptr<ASTEqExp> eq_exp;
    std::shared_ptr<ASTRelExp> rel_exp;
    RelOp op;
}
;
struct ASTLAndExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTLAndExp() = default;
    std::shared_ptr<ASTLAndExp> l_and_exp;
    std::shared_ptr<ASTEqExp> eq_exp;
    LogOp op;
}
;
struct ASTLOrExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTLOrExp() = default;
    std::shared_ptr<ASTLOrExp> l_or_exp;
    std::shared_ptr<ASTLAndExp> l_and_exp;
    LogOp op;
}
;
struct ASTConstExp : ASTNode{
    virtual  Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstExp() = default;
    std::shared_ptr<ASTAddExp> add_exp;
}
;

class ASTVisitor {
    public:
       // virtual Value *visit(ASTNode &) = 0;
        virtual Value *visit(ASTCompUnit &) = 0;
        virtual Value *visit(ASTDecl &) = 0;
        virtual Value *visit(ASTConstDecl &) = 0;
    //    virtual Value *visit(ASTConstDefList &) = 0;
        virtual Value *visit(ASTConstDef &) = 0;
        virtual Value *visit(ASTConstInitVal &) = 0;
        virtual Value *visit(ASTVarDecl &) = 0;
        virtual Value *visit(ASTVarDef &) = 0;
       // virtual Value *visit(ASTVarDefList &) = 0;
        virtual Value *visit(ASTInitVal &) = 0;
        virtual Value *visit(ASTFuncDef &) = 0;
        //virtual Value *visit(ASTFuncFParams &) = 0;
        virtual Value *visit(ASTFuncFParam &) = 0;
        virtual Value *visit(ASTBlock &) = 0;
        virtual Value *visit(ASTBlockItem &) = 0;
        virtual Value *visit(ASTStmt &) = 0;
        virtual Value *visit(ASTExp &) = 0;
        virtual Value *visit(ASTCond &) = 0;
        virtual Value *visit(ASTLVal &) = 0;
        virtual Value *visit(ASTPrimaryExp &) = 0;
        virtual Value *visit(ASTNumber &) = 0;
        virtual Value *visit(ASTUnaryExp &) = 0;
        virtual Value *visit(ASTMulExp &) = 0;
        virtual Value *visit(ASTAddExp &) = 0;
        virtual Value *visit(ASTRelExp &) = 0;
        virtual Value *visit(ASTEqExp &) = 0;
        virtual Value *visit(ASTLAndExp &) = 0;
        virtual Value *visit(ASTLOrExp &) = 0;
        virtual Value *visit(ASTConstExp &) = 0;
        //virtual Value *visit(ASTUnaryExp &) = 0;
       // virtual Value *visit(ASTFuncType &) = 0;
       // virtual Value *visit(ASTConstArrayIdent &) = 0;
        //virtual Value *visit(ASTInitValArrayList &) = 0;
        //virtual Value *visit(ASTFuncArrayIdent &) = 0;
      //  virtual Value *visit(ASTArrayIdent &) = 0;
        virtual Value *visit(ASTAssignStmt &) = 0;
        virtual Value *visit(ASTSelectionStmt &) = 0;
        virtual Value *visit(ASTIterationStmt &) = 0;
        virtual Value *visit(ASTReturnStmt &) = 0;
        virtual Value *visit(ASTBreakStmt &) = 0;
        virtual Value *visit(ASTContinueStmt &) = 0;
        virtual Value *visit(ASTCall &) = 0;
};

class ASTPrinter : public ASTVisitor {
    public:
      //  virtual Value *visit(ASTNode &) override final;
        virtual Value *visit(ASTCompUnit &) override final;
        virtual Value *visit(ASTDecl &) override final;
        virtual Value *visit(ASTConstDecl &) override final;
       // virtual Value *visit(ASTConstDefList &) override final;
        virtual Value *visit(ASTConstDef &) override final;
        virtual Value *visit(ASTConstInitVal &) override final;
        virtual Value *visit(ASTVarDecl &) override final;
        virtual Value *visit(ASTVarDef &) override final;
      //  virtual Value *visit(ASTVarDefList &) override final;
        virtual Value *visit(ASTInitVal &) override final;
        virtual Value *visit(ASTFuncDef &) override final;
        //virtual Value *visit(ASTFuncFParams &) override final;
        virtual Value *visit(ASTFuncFParam &) override final;
        virtual Value *visit(ASTBlock &) override final;
        virtual Value *visit(ASTBlockItem &) override final;
        virtual Value *visit(ASTStmt &) override final;
        virtual Value *visit(ASTExp &) override final;
        virtual Value *visit(ASTCond &) override final;
        virtual Value *visit(ASTLVal &) override final;
        virtual Value *visit(ASTPrimaryExp &) override final;
        virtual Value *visit(ASTNumber &) override final;
        virtual Value *visit(ASTUnaryExp &) override final;
        virtual Value *visit(ASTMulExp &) override final;
        virtual Value *visit(ASTAddExp &) override final;
        virtual Value *visit(ASTRelExp &) override final;
        virtual Value *visit(ASTEqExp &) override final;
        virtual Value *visit(ASTLAndExp &) override final;
        virtual Value *visit(ASTLOrExp &) override final;
        virtual Value *visit(ASTConstExp &) override final;
      //  virtual Value *visit(ASTFuncType &) override final;
      //  virtual Value *visit(ASTConstArrayIdent &) override final;
       // virtual Value *visit(ASTInitValArrayList &) override final;
       // virtual Value *visit(ASTFuncArrayIdent &) override final;
       // virtual Value *visit(ASTArrayIdent &) override final;
        virtual Value *visit(ASTAssignStmt &) override final;
        virtual Value *visit(ASTSelectionStmt &) override final;
        virtual Value *visit(ASTIterationStmt &) override final;
        virtual Value *visit(ASTReturnStmt &) override final;
        virtual Value *visit(ASTBreakStmt &) override final;
        virtual Value *visit(ASTContinueStmt &) override final;
        virtual Value *visit(ASTCall &) override final;
        void add_depth() { depth += 2; }
        void remove_depth() { depth -= 2; }

  private:
    int depth = 0;
};