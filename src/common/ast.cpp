#include "ast.hpp"
#include "Value.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <stack>

#define _AST_NODE_ERROR_                                                       \
    std::cerr << "Abort due to node cast error."                               \
                 "Contact with TAs to solve your problem."                     \
              << std::endl;                                                    \
    std::abort();
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

void AST::run_visitor(ASTVisitor &visitor) { root->accept(visitor); }

AST::AST(syntax_tree *s) {
    if (s == nullptr) {
        std::cerr << "empty input tree!" << std::endl;
        std::abort();
    }
    auto node = transform_node_iter(s->root);
    del_syntax_tree(s);
    root = std::shared_ptr<ASTCompUnit>(static_cast<ASTCompUnit *>(node));
}

ASTNode *AST::transform_node_iter(syntax_tree_node *n) {
    /*
    CompUnit : Decl { $$ = node("CompUnit", 1, $1);gt->root = $$;}
         | CompUnit Decl { $$ = node("CompUnit", 2, $1, $2); }
         ;
    */
    if(_STR_EQ(n->name, "CompUnit")) {
        auto node = new ASTCompUnit();
        //node->lineno = n->lineno;
        std::stack<syntax_tree_node *> s;
        auto list_ptr = n;
       // //printf("%d   children_num of CompUnit\n",list_ptr->children_num);
        while (list_ptr->children_num == 2) {
            s.push(list_ptr->children[1]);
            list_ptr = list_ptr->children[0];
            ////printf("%d   children_num of CompUnit\n",list_ptr->children_num);
        }
        ////printf("%d   children_num of CompUnit\n",list_ptr->children_num);
        ////printf("%s   name of CompUnit\n",list_ptr->children[0]->name);
        s.push(list_ptr->children[0]);

        while (!s.empty()) {
            auto child_node = static_cast<ASTDecl *>(transform_node_iter(s.top()));
            if(child_node == nullptr){
                _AST_NODE_ERROR_;
            }
            auto child_node_ptr = std::shared_ptr<ASTDecl>(child_node);
            node->declarations.push_back(child_node_ptr);
            //printf("pushed a decl\n");
            s.pop();
        }
        return node;        
    }
    /*
    Decl : ConstDecl { $$ = node("Decl", 1, $1); }
        | VarDecl { $$ = node("Decl", 1, $1); }
        | FuncDef { $$ = node("Decl", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name, "Decl" )){
        auto node = new ASTDecl();
        if(_STR_EQ(n->children[0]->name ,"ConstDecl")){
            //printf("ConstDecl\n");
            auto const_decl = static_cast<ASTConstDecl *>(transform_node_iter(n->children[0]));
            node->const_decl = std::shared_ptr<ASTConstDecl>(const_decl);
        }
        else if(_STR_EQ(n->children[0]->name , "VarDecl")){
            ////printf("VarDecl\n");
            auto var_decl = static_cast<ASTVarDecl *>(transform_node_iter(n->children[0]));
            node->var_decl = std::shared_ptr<ASTVarDecl>(var_decl);
        }
        else if(_STR_EQ(n->children[0]->name , "FuncDef")){
            ////printf("FuncDef\n");
            auto func_def = static_cast<ASTFuncDef *>(transform_node_iter(n->children[0]));
            node->func_def = std::shared_ptr<ASTFuncDef>(func_def);
        }
        else{
            _AST_NODE_ERROR_
        }       
        return node;
    }
    /*
    ConstDecl : CONST BType ConstDefList SEMICOLON { $$ = node("ConstDecl", 4,$1, $2, $3, $4); }
        ;
    
    ConstDefList : ConstDefList COMMA ConstDef { $$ = node("ConstDef", 3, $1, $2, $3); }
        | ConstDef { $$ = node("ConstDef", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name, "ConstDecl")){
        auto node = new ASTConstDecl();
        if(_STR_EQ(n->children[1]->name,"BType"))
        {
            if(_STR_EQ(n->children[1]->children[0]->name,"int")){
                //printf("int\n");
                node->type = TYPE_INT;
            }
            else if(_STR_EQ(n->children[1]->children[0]->name,"float")){
                 //printf("float\n");
                node->type = TYPE_FLOAT;
            }
            else{
                _AST_NODE_ERROR_
            }        
        }
        ////printf("    enter const\n");
        auto list_ptr = n->children[2];
        std::stack<syntax_tree_node *> s;
        if(list_ptr->children_num == 3)
        {
            while(list_ptr->children_num == 3)
            {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
        }
        s.push(list_ptr->children[0]);

        while (!s.empty()) {
            auto child_node = static_cast<ASTConstDef *>(transform_node_iter(s.top()));
            if(child_node == nullptr){
                _AST_NODE_ERROR_;
            }
            //printf("pushed a constdef\n");
            auto child_node_ptr = std::shared_ptr<ASTConstDef>(child_node);
            node->const_defs.push_back(child_node_ptr);
            s.pop();
        }
        return node;
    }
    /*
    ConstDef : ID ASSIGN ConstInitVal { $$ = node("ConstDef", 3, $1, $2, $3); }
        |  ID ConstArrayIdent ASSIGN ConstInitVal { $$ = node("ConstDef", 4, $1, $2, $3,$4);}
        ;
    */
    else if(_STR_EQ(n->name, "ConstDef")){
        auto node = new ASTConstDef();
        node->id = n->children[0]->name;
        ////printf("%s          constdef\n",n->children[0]->name);
    //    node->const_array_ident = nullptr;
        if(_STR_EQ(n->children[1]->name,"ConstArrayIdent")){
            
            auto const_array_ident = static_cast<ASTConstArrayIdent *>(transform_node_iter(n->children[
                1]));
            node->const_array_ident = std::shared_ptr<ASTConstArrayIdent>(const_array_ident);
            auto const_init_val = static_cast<ASTConstInitVal *>(transform_node_iter(n->children[3]));
            node->const_init_val = std::shared_ptr<ASTConstInitVal>(const_init_val);
        }
        else{
            //printf("constdef 3\n");
            auto const_init_val = static_cast<ASTConstInitVal *>(transform_node_iter(n->children[2]));
            node->const_init_val = std::shared_ptr<ASTConstInitVal>(const_init_val);
        }
        return node;
    }
    /*
    ConstArrayIdent :  LBRACKET constExp RBRACKET{ $$ = node("ConstArrayIdent", 0); }
        | ConstArrayIdent LBRACKET constExp RBRACKET { $$ = node("ConstArrayIdent", 4, $1, $2, $3, $4); }
        ;
    */
    else if(_STR_EQ(n->name, "ConstArrayIdent")){
        auto node = new ASTConstArrayIdent();
        auto list_ptr = n;
        std::stack<syntax_tree_node *> s;
        if(list_ptr->children_num == 4){
            while(list_ptr->children_num == 4){
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
        }
        s.push(list_ptr->children[1]);
        while(!s.empty()){
            auto child_node = static_cast<ASTConstExp *>(transform_node_iter(s.top()));
            ////printf("        const array\n");
            if(child_node == nullptr){
                _AST_NODE_ERROR_;
            }
            auto child_node_ptr = std::shared_ptr<ASTConstExp>(child_node);
            node->const_exps.push_back(child_node_ptr);
            s.pop();
        }
        return node;
    }
    /*
    ConstInitVal : ConstExp { $$ = node("ConstInitVal", 1, $1); }
        | LBRACE ConstInitValArrayList RBRACE { $$ = node("ConstInitVal", 3,$1, $2, $3); }
        | LBRACE RBRACE { $$ = node("ConstInitVal", 2, $1, $2); }
        ;

    ConstInitValArrayList : ConstInitValArrayList COMMA ConstInitVal { $$ = node("ConstInitValArrayList", 3, $1, $2, $3); }
        | ConstInitVal { $$ = node("ConstInitValArrayList", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name, "ConstInitVal")){
        auto node = new ASTConstInitVal();
        if(_STR_EQ(n->children[0]->name,"ConstExp")){
            //printf("constexp\n");
            auto const_exp = static_cast<ASTConstExp *>(transform_node_iter(n->children[0]));
            node->const_exp = std::shared_ptr<ASTConstExp>(const_exp);
        }
        else if(_STR_EQ(n->children[1]->name , "ConstInitValArrayList")){
            auto list_ptr = n->children[1];
            std::stack<syntax_tree_node *> s;
            if(list_ptr->children_num == 3){
                while(list_ptr->children_num == 3){
                    s.push(list_ptr->children[2]);
                    list_ptr = list_ptr->children[0];
                }
            }
            s.push(list_ptr->children[0]);  
            while(!s.empty()){
                auto child_node = static_cast<ASTConstInitVal *>(transform_node_iter(s.top()));
                if(child_node == nullptr){
                    _AST_NODE_ERROR_;
                }
                auto child_node_ptr = std::shared_ptr<ASTConstInitVal>(child_node);
                node->const_init_vals.push_back(child_node_ptr);
                s.pop();
            }
        }
        return node;
    }
    /*
    VarDecl : BType VarDefList SEMICOLON { $$ = node("VarDecl", 3, $1, $2, $3); }

    VarDefList : VarDefList COMMA VarDef { $$ = node("VarDefList", 3, $1, $2, $3); }
        | VarDef { $$ = node("VarDefList", 1, $1); }
        ;
    BType : INT { $$ = node("BType", 1, $1); }
        | FLOAT { $$ = node("BType", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"VarDecl"))
    {
        auto node = new ASTVarDecl();
        if(_STR_EQ(n->children[0]->children[0]->name , "int"))
        {
            ////printf("int\n");
            node->type = TYPE_INT;
        }else if(_STR_EQ(n->children[0]->children[0]->name,"float")){
            ////printf("float\n");
            node->type = TYPE_FLOAT;
        }
        else{
            _AST_NODE_ERROR_
        }          
        auto list_ptr = n->children[1];
        std::stack<syntax_tree_node *> s;
        if(list_ptr->children_num == 3)
        {
            while(list_ptr->children_num == 3)
            {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
        }
        ////printf("%d   children_num of VarDefList\n",list_ptr->children_num);
        s.push(list_ptr->children[0]);

        while (!s.empty()) {
            auto child_node = static_cast<ASTVarDef *>(transform_node_iter(s.top()));
            if(child_node == nullptr){
                _AST_NODE_ERROR_;
            }
            auto child_node_ptr = std::shared_ptr<ASTVarDef>(child_node);
            node->var_defs.push_back(child_node_ptr);
            s.pop();
        }

        return node;
    }
    /*
    VarDef : ID { $$ = node("VarDef", 1, $1); }
        | ID ConstArrayIdent { $$ = node("VarDef", 1, $1); }
        | ID ASSIGN Exp { $$ = node("VarDef", 3, $1, $2, $3); }
        | ID ConstArrayIdent ASSIGN InitVal { $$ = node("VarDef", 4, $1, $2, $3, $4); }
    */
    else if(_STR_EQ(n->name,"VarDef"))
    {
        auto node = new ASTVarDef();
        node->id = n->children[0]->name;
        ////printf("%s    vardef\n",n->children[0]->name);
        if(n->children_num == 2){
            auto array_ident = static_cast<ASTConstArrayIdent *>(transform_node_iter(n->children[1]));
            node->array_ident = std::shared_ptr<ASTConstArrayIdent>(array_ident);
        }
        else if(n->children_num == 3){
            auto init_val = new ASTInitVal();
            auto exp  = static_cast<ASTExp *>(transform_node_iter(n->children[2]));
            init_val->exp = std::shared_ptr<ASTExp>(exp);
            node->init_val = std::shared_ptr<ASTInitVal>(init_val);
        }
        else if(n->children_num == 4){
            ////printf("vardef 4\n");
            auto array_ident = static_cast<ASTConstArrayIdent *>(transform_node_iter(n->children[1]));
            node->array_ident = std::shared_ptr<ASTConstArrayIdent>(array_ident);
            auto init_val = static_cast<ASTInitVal *>(transform_node_iter(n->children[3]));
            node->init_val = std::shared_ptr<ASTInitVal>(init_val);
        }
        else{
            
        }

        return node;
    }
    /*
    InitVal :  Exp { $$ = node("InitVal", 1, $1); }
        | LBRACE InitValArrayList RBRACE { $$ = node("InitVal", 3, $1, $2, $3); }
        | LBRACE RBRACE { $$ = node("InitVal", 2, $1, $2); }
        ;

    InitValArrayList : InitValArrayList COMMA InitVal { $$ = node("InitValArrayList", 3, $1, $2, $3); }
        | InitVal { $$ = node("InitValArrayList",1,$1);}
        ;
    */
    else if(_STR_EQ(n->name, "InitVal"))
    {
        auto node = new ASTInitVal();
        if(n->children_num == 1)
        {
            if(_STR_EQ(n->children[0]->name,"Exp"))
            {
                auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
                node->exp = std::shared_ptr<ASTExp>(exp);
            }    
            return node;
        }
        else if(n->children_num == 3)
        {
            auto list_ptr = n->children[1];
            std::stack<syntax_tree_node *> s;
            if(list_ptr->children_num == 3)
            {
                while(list_ptr->children_num == 3)
                {
                    ////printf("enter initvalarraylist\n");
                    s.push(list_ptr->children[2]);
                    list_ptr = list_ptr->children[0];
                }
            }
            s.push(list_ptr->children[0]);  
            while(!s.empty())
            {
                auto child_node = static_cast<ASTInitVal *>(transform_node_iter(s.top()));
                if(child_node == nullptr)
                {
                    _AST_NODE_ERROR_;
                }
                auto child_node_ptr = std::shared_ptr<ASTInitVal>(child_node);
                ////printf("pushed a initval\n");
                node->init_vals.push_back(child_node_ptr);
                s.pop();
            }
        }
    
        return node;
    }
    /*
    
    FuncDef :   BType ID LPARENTHESIS FuncFParams RPARENTHESIS Block { $$ = node("FuncDef", 6, $1, $2, $3, $4, $5, $6); }
        |   BType ID LPARENTHESIS RPARENTHESIS Block { $$ = node("FuncDef", 5, $1, $2, $3, $4, $5); }
        |   VOID ID LPARENTHESIS FuncFParams RPARENTHESIS Block { $$ = node("FuncDef", 6, $1, $2, $3, $4, $5, $6); }
        |   VOID ID LPARENTHESIS RPARENTHESIS Block { $$ = node("FuncDef", 5, $1, $2, $3, $4, $5); }
        ;
    
    FuncFParams :   FuncFParams COMMA FuncFParam { $$ = node("FuncFParams", 3, $1, $2, $3); }
        |   FuncFParam { $$ = node("FuncFParams", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name, "FuncDef"))
    {
        auto node = new ASTFuncDef();
         ////printf("%s\n",n->children[1]->name);
        if(_STR_EQ(n->children[0]->name,"BType"))
        {
            ////printf("BType\n");
            if(_STR_EQ(n->children[0]->children[0]->name, "int")){
                ////printf("int\n");
                node->type = TYPE_INT;
            }
            else if(_STR_EQ(n->children[0]->children[0]->name ,"float")){
                ////printf("float\n");
                node->type = TYPE_FLOAT;
            }
        }
        else if(_STR_EQ(n->children[0]->name, "void")){
                ////printf("void\n");
                node->type = TYPE_VOID;
        }
        ////printf("before\n");
        node->id = n->children[1]->name;
        ////printf("%s\n",n->children[1]->name);
        if(n->children_num == 6)
        {
            auto list_ptr = n->children[3];
            std::stack<syntax_tree_node *> s;
            if(list_ptr->children_num == 3)
            {
                while(list_ptr->children_num == 3)
                {
                    s.push(list_ptr->children[2]);
                    list_ptr = list_ptr->children[0];
                }
            }
            ////printf("%d    children_num of Funcfparam\n",list_ptr->children_num);
            s.push(list_ptr->children[0]);

            while (!s.empty()) {
                auto child_node = static_cast<ASTFuncFParam *>(transform_node_iter(s.top()));
                if(child_node == nullptr){
                    _AST_NODE_ERROR_;
                }
                auto child_node_ptr = std::shared_ptr<ASTFuncFParam>(child_node);
                node->func_fparams.push_back(child_node_ptr);
                s.pop();
            }
        }
        auto block = static_cast<ASTBlock *>(transform_node_iter(n->children[n->children_num - 1]));
        node->block = std::shared_ptr<ASTBlock>(block);
    
        return node;
    }
    /*
    FuncFParam :    BType ID { $$ = node("FuncFParam", 2, $1, $2); }
        |   BType ID LBRACKET RBRACKET FuncArrayIdent { $$ = node("FuncFParam", 2, $1, $2); }
        ;
    */
    else if(_STR_EQ(n->name,"FuncFParam"))
    {
        auto node = new ASTFuncFParam();
        node->isarray = false;
     //   node->array_ident = nullptr;
        if(_STR_EQ(n->children[0]->name,"BType"))
        {
            if(_STR_EQ(n->children[0]->children[0]->name , "int")){
                node->type = TYPE_INT;
            }
            else if(_STR_EQ(n->children[0]->children[0]->name,"float")){
                node->type = TYPE_FLOAT;
            }
            else{
                _AST_NODE_ERROR_
            }        
        }
        node->id = n->children[1]->name;
        ////printf("%s\n",n->children[1]->name);
        if(n->children_num == 5)
        {
            node->isarray = true;
            auto array_ident =  static_cast<ASTFuncArrayIdent *>(transform_node_iter(n->children[4]));
            node->array_ident = std::shared_ptr<ASTFuncArrayIdent>(array_ident);
        }

        return node;
    }
    /*
    FuncArrayIdent :  FuncArrayIdent LBRACKET Exp RBRACKET { $$ = node("FuncArrayIdent", 4, $1, $2, $3, $4); } 
        |   empty { $$ = node("FuncArrayIdent", 0); }
        ;
    */
    else if(_STR_EQ(n->name,"FuncArrayIdent"))
    {
        auto node = new ASTFuncArrayIdent();
        auto list_ptr = n;
        std::stack<syntax_tree_node *> s;
        if(list_ptr->children_num == 4)
        {
            while(list_ptr->children_num == 4)
            {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
        }
        
        while(!s.empty())
        {
            auto child_node = static_cast<ASTExp *>(transform_node_iter(s.top()));
            if(child_node == nullptr)
            {
                _AST_NODE_ERROR_;
            }
            auto child_node_ptr = std::shared_ptr<ASTExp>(child_node);
            node->exps.push_back(child_node_ptr);
            s.pop();
        }
        return node;
    }
    /*
    Block : LBRACE BlockItemList RBRACE { $$ = node("Block", 3, $1, $2, $3); }
        | LBRACE RBRACE { $$ = node("Block", 2, $1, $2); }
        ;

    BlockItemList : BlockItemList BlockItem { $$ = node("BlockItemList", 2, $1, $2); }
        | BlockItem { $$ = node("BlockItemList", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"Block"))
    {
        auto node = new ASTBlock();
        if(n->children_num == 3)
        {
            auto list_ptr = n->children[1];
            std::stack<syntax_tree_node *> s;
            if(list_ptr->children_num == 2)
            {
                ////printf("enter block\n");
                while(list_ptr->children_num == 2)
                {
                    s.push(list_ptr->children[1]);
                    list_ptr = list_ptr->children[0];
                }
            }
            ////printf("%d\n ",list_ptr->children_num);
            s.push(list_ptr->children[0]);

            while (!s.empty()) {
                auto child_node = static_cast<ASTBlockItem *>(transform_node_iter(s.top()));
                if(child_node == nullptr){
                    _AST_NODE_ERROR_;
                }
                auto child_node_ptr = std::shared_ptr<ASTBlockItem>(child_node);
                node->block_items.push_back(child_node_ptr);
                s.pop();
            }
        }
        return node;
    }
    /*
    BlockItem : Stmt { $$ = node("BlockItem", 1, $1); }
        | ConstDecl { $$ = node("BlockItem", 1, $1); }
        | VarDecl { $$ = node("BlockItem", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"BlockItem"))
    {
        auto node = new ASTBlockItem();
        if(_STR_EQ(n->children[0]->name,"Stmt"))
        {
            ////printf("stmt\n");
            auto stmt = static_cast<ASTStmt *>(transform_node_iter(n->children[0]));
            node->stmt = std::shared_ptr<ASTStmt>(stmt);
        }
        else if(_STR_EQ(n->children[0]->name,"ConstDecl"))
        {
            auto const_decl = static_cast<ASTConstDecl *>(transform_node_iter(n->children[0]));
            node->const_decl = std::shared_ptr<ASTConstDecl>(const_decl);
        }
        else if(_STR_EQ(n->children[0]->name,"VarDecl"))
        {
            auto var_decl = static_cast<ASTVarDecl *>(transform_node_iter(n->children[0]));
            node->var_decl = std::shared_ptr<ASTVarDecl>(var_decl);
        }
        else{
            _AST_NODE_ERROR_
        }
        return node;
    }
    /*
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
    */
    else if(_STR_EQ(n->name,"Stmt"))
    {
        auto node = new ASTStmt();
        if(_STR_EQ(n->children[0]->name,"LVal"))
        {
            ////printf("lval\n");
            auto lval = static_cast<ASTLVal *>(transform_node_iter(n->children[0]));
            auto assign_stmt = new ASTAssignStmt();
            assign_stmt->l_val = std::shared_ptr<ASTLVal>(lval);
            ////printf("exp\n");
            auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[2]));
            assign_stmt->exp = std::shared_ptr<ASTExp>(exp);
            node->assign_stmt = std::shared_ptr<ASTAssignStmt>(assign_stmt);
        }
        else if(_STR_EQ(n->children[0]->name , "Exp"))
        {
            auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
            node->exp = std::shared_ptr<ASTExp>(exp);
        }
        else if(_STR_EQ(n->children[0]->name,"Block"))
        {
            auto block = static_cast<ASTBlock *>(transform_node_iter(n->children[0]));
            node->block = std::shared_ptr<ASTBlock>(block);
        }
        else if(_STR_EQ(n->children[0]->name,"if"))
        {
            auto selection_stmt = new ASTSelectionStmt();
            ////printf("if  s\n");
            auto cond = static_cast<ASTCond *>(transform_node_iter(n->children[2]));
            selection_stmt->cond = std::shared_ptr<ASTCond>(cond);
            ////printf("cond\n");
            auto stmt1 = static_cast<ASTStmt *>(transform_node_iter(n->children[4]));
            selection_stmt->if_stmt = std::shared_ptr<ASTStmt>(stmt1);
            ////printf("else\n");
            if(n->children_num == 7)
            {
                auto stmt2 = static_cast<ASTStmt *>(transform_node_iter(n->children[6]));
                selection_stmt->else_stmt = std::shared_ptr<ASTStmt>(stmt2);
            }
            node->selection_stmt = std::shared_ptr<ASTSelectionStmt>(selection_stmt);
        }
        else if(_STR_EQ(n->children[0]->name,"while"))
        {
            auto iteration_stmt = new ASTIterationStmt();
            auto cond = static_cast<ASTCond *>(transform_node_iter(n->children[2]));
            iteration_stmt->cond = std::shared_ptr<ASTCond>(cond);
            auto stmt = static_cast<ASTStmt *>(transform_node_iter(n->children[4]));
            iteration_stmt->stmt = std::shared_ptr<ASTStmt>(stmt);
            node->iteration_stmt = std::shared_ptr<ASTIterationStmt>(iteration_stmt);
        }
        else if(_STR_EQ(n->children[0]->name,"return"))
        {
            auto return_stmt = new ASTReturnStmt();
            if(n->children_num == 3)
            {
                auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[1]));
                return_stmt->exp = std::shared_ptr<ASTExp>(exp);
            }
            node->return_stmt = std::shared_ptr<ASTReturnStmt>(return_stmt);
        }
        else if(_STR_EQ(n->children[0]->name,"break"))
        {
            auto break_stmt = new ASTBreakStmt();
            node->break_stmt = std::shared_ptr<ASTBreakStmt>(break_stmt);
        }
        else if(_STR_EQ(n->children[0]->name,"continue"))
        {
            auto continue_stmt = new ASTContinueStmt();
            node->continue_stmt = std::shared_ptr<ASTContinueStmt>(continue_stmt);
        }
        else{
           // return nullptr;
        }
        
        return node;
    }
    /*
    Exp : AddExp { $$ = node("Exp", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"Exp"))
    {
        auto node = new ASTExp();
        auto add_exp = static_cast<ASTAddExp *>(transform_node_iter(n->children[0]));
        node->add_exp = std::shared_ptr<ASTAddExp>(add_exp);

        return node;
    }
    /*
    Cond : LOrExp { $$ = node("Cond", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"Cond"))
    {
        auto node = new ASTCond();
      //  //printf("l_or_exp\n");
        auto l_or_exp = static_cast<ASTLOrExp *>(transform_node_iter(n->children[0]));
        node->l_or_exp = std::shared_ptr<ASTLOrExp>(l_or_exp); 

        return node;
    }
    /*
    LVal : ID { $$ = node("LVal", 1, $1); }
        |  ID ArrayIdent { $$ = node("LVal", 2, $1, $2); }
        ;
    */
    else if(_STR_EQ(n->name,"LVal"))
    {
        auto node = new ASTLVal();
        //printf("%s\n",n->children[0]->name);
        node->id = n->children[0]->name;
        //node->array_ident = nullptr;
        if(n->children_num == 2)
        {
            auto array_ident = static_cast<ASTArrayIdent *>(transform_node_iter(n->children[1]));
            node->array_ident = std::shared_ptr<ASTArrayIdent>(array_ident);
        }

        return node;
    }
    /*
    ArrayIdent: ArrayIdent LBRACKET Exp RBRACKET { $$ = node("ArrayIdent", 4, $1, $2, $3, $4); }
        |LBRACKET Exp RBRACKET{ $$ = node("ArrayIdent", 0); }
        ;
    */
    else if(_STR_EQ(n->name,"ArrayIdent"))
    {
        auto node = new ASTArrayIdent();
        auto list_ptr = n;
        std::stack<syntax_tree_node *> s;
        if(list_ptr->children_num == 4)
        {
            while(list_ptr->children_num == 4)
            {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
        }
        s.push(list_ptr->children[1]);
        while(!s.empty())
        {
            auto child_node = static_cast<ASTExp *>(transform_node_iter(s.top()));
            if(child_node == nullptr)
            {
                _AST_NODE_ERROR_;
            }
            auto child_node_ptr = std::shared_ptr<ASTExp>(child_node);
            node->exps.push_back(child_node_ptr);
            s.pop();
        }
        return node;
    }
    /*
    PrimaryExp : LPARENTHESIS Exp RPARENTHESIS { $$ = node("PrimaryExp", 3, $1, $2, $3); }
        | LVal { $$ = node("PrimaryExp", 1, $1); }
        | Number { $$ = node("PrimaryExp", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"PrimaryExp"))
    {
        auto node = new ASTPrimaryExp();
        if(n->children_num == 3)
        {
            auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[1]));
            node->exp = std::shared_ptr<ASTExp>(exp);
        }
        else if(_STR_EQ(n->children[0]->name ,"LVal"))
        {
            auto lval = static_cast<ASTLVal *>(transform_node_iter(n->children[0]));
            node->l_val = std::shared_ptr<ASTLVal>(lval);
        }
        else if(_STR_EQ(n->children[0]->name , "Number"))
        {
            //printf("number\n");
            auto number = static_cast<ASTNumber *>(transform_node_iter(n->children[0]));
            node->number = std::shared_ptr<ASTNumber>(number);
        }
        else{
            _AST_NODE_ERROR_
        }

        return node;
    }
    /*
    Number :  INTEGER { $$ = node("Number", 1, $1); }
        | FLOATPOINT { $$ = node("Number", 1, $1); }
        ;

    INTEGER:  INTEGER_10 { $$ = node("Integer", 1, $1); }
        | INTEGER_8 { $$ = node("Integer", 1, $1); }
        | INTEGER_16 { $$ = node("Integer", 1, $1); }
        ;
    FLOATPOINT:        FLOATPOINT_10 { $$ = node("FloatPoint", 1, $1); }
        | FLOATPOINT_16 { $$ = node("FloatPoint", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"Number"))
    {
        auto node = new ASTNumber();
        //printf("%s\n",n->children[0]->name);
        if(_STR_EQ(n->children[0]->name,"Integer"))
        {
            //printf("integer   %s\n",n->children[0]->children[0]->name);
            node->type = TYPE_INT;
            if(n->children[0]->children[0]->name[1] == 'x' || n->children[0]->children[0]->name[1] == 'X')
            {
                node->int_val = std::stoi(n->children[0]->children[0]->name,nullptr,16);
            }
            else if(n->children[0]->children[0]->name[0] == '0' and !_STR_EQ(n->children[0]->children[0]->name,"0"))
            {
                node->int_val = std::stoi(n->children[0]->children[0]->name,nullptr,8);
            }
            else
            {
            node->int_val = std::stoi(n->children[0]->children[0]->name);
            }
        }
        else if(_STR_EQ(n->children[0]->name,"FloatPoint"))
        {
            //printf("floatpoint  %s\n",n->children[0]->children[0]->name);
            node->type = TYPE_FLOAT;
            node->float_val = std::stof(n->children[0]->children[0]->name);
            return node;
        }
        return node;
    }
    /*
    UnaryExp : PrimaryExp { $$ = node("UnaryExp", 1, $1); }
        | UnaryOp UnaryExp { $$ = node("UnaryExp", 2, $1, $2); }
        | ID LPARENTHESIS FuncRParams RPARENTHESIS { $$ = node("UnaryExp", 4, $1, $2, $3, $4); }
        | ID LPARENTHESIS RPARENTHESIS { $$ = node("UnaryExp", 3, $1, $2, $3); }
        ;
    
    FuncRParams : FuncRParams COMMA Exp { $$ = node("FuncRParams", 3, $1, $2, $3); }
        | Exp { $$ = node("FuncRParams", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"UnaryExp"))
    {
        auto node = new ASTUnaryExp();
        ////printf("%s UnaryExp\n",n->children[0]->name);
        if(_STR_EQ(n->children[0]->name,"PrimaryExp"))
        {
            //printf("primary\n");
            auto primary_exp = static_cast<ASTPrimaryExp *>(transform_node_iter(n->children[0]));
            node->primary_exp = std::shared_ptr<ASTPrimaryExp>(primary_exp);
           // if(node->unary_exp != nullptr) ////printf( "unary_exp is not null");
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "UnaryOp"))
        {
            if(_STR_EQ(n->children[0]->children[0]->name , "-"))
            {
                node->op = OP_NEG;
            }
            else if(_STR_EQ(n->children[0]->children[0]->name , "!"))
            {
                node->op = OP_NOT;
            }
            else if(_STR_EQ(n->children[0]->children[0]->name , "+"))
            {
                node->op = OP_POS;
            }
            else{
                _AST_NODE_ERROR_
            }
            auto unary_exp = static_cast<ASTUnaryExp *>(transform_node_iter(n->children[1]));
            node->unary_exp = std::shared_ptr<ASTUnaryExp>(unary_exp);
            return node;
        }
        else 
        {
            auto call = new ASTCall();
            call->lineno = n->lineno;
            call->id = n->children[0]->name;
            if(n->children_num == 4)
            {
                auto list_ptr = n->children[2];
                std::stack<syntax_tree_node *> s;
                if(list_ptr->children_num == 3)
                {
                    while(list_ptr->children_num == 3)
                    {
                        s.push(list_ptr->children[2]);
                        list_ptr = list_ptr->children[0];
                    }
                }
                s.push(list_ptr->children[0]);

                while (!s.empty()) {
                    auto child_node = static_cast<ASTExp *>(transform_node_iter(s.top()));
                    if(child_node == nullptr){
                        _AST_NODE_ERROR_;
                    }
                    auto child_node_ptr = std::shared_ptr<ASTExp>(child_node);
                    call->exps.push_back(child_node_ptr);
                    s.pop();
                }
            }
            node->call = std::shared_ptr<ASTCall>(call);
        }

        return node;
    }
    /*
    MulExp : UnaryExp { $$ = node("MulExp", 1, $1); }
        | MulExp MUL UnaryExp { $$ = node("MulExp", 3, $1, $2, $3); }
        | MulExp DIV UnaryExp { $$ = node("MulExp", 3, $1, $2, $3); }
        | MulExp MOD UnaryExp { $$ = node("MulExp", 3, $1, $2, $3); }
        ;
    */
    else if(_STR_EQ(n->name,"MulExp"))
    {
        auto node = new ASTMulExp();
        if(n->children_num == 1)
        {
            //printf("mul 1\n");
            auto unary_exp = static_cast<ASTUnaryExp *>(transform_node_iter(n->children[0]));
            node->unary_exp = std::shared_ptr<ASTUnaryExp>(unary_exp);
        }
        else if(n->children_num == 3)
        {
            auto mul_exp = static_cast<ASTMulExp *>(transform_node_iter(n->children[0]));
            node->mul_exp = std::shared_ptr<ASTMulExp>(mul_exp);
            auto unary_exp = static_cast<ASTUnaryExp *>(transform_node_iter(n->children[2]));
            node->unary_exp = std::shared_ptr<ASTUnaryExp>(unary_exp);
            if(_STR_EQ(n->children[1]->name,"*"))
            {
                node->op = OP_MUL;
            }
            else if(_STR_EQ(n->children[1]->name,"/"))
            {
                node->op = OP_DIV;
            }
            else if(_STR_EQ(n->children[1]->name,"%"))
            {
                node->op = OP_MOD;
            }
            else{
                _AST_NODE_ERROR_
            }
        }
        else{
            _AST_NODE_ERROR_
        }
        return node;
    }
    /*
    AddExp : MulExp { $$ = node("AddExp", 1, $1); }
        | AddExp ADD MulExp { $$ = node("AddExp", 3, $1, $2, $3); }
        | AddExp SUB MulExp { $$ = node("AddExp", 3, $1, $2, $3); }
        ;
    */
    else if(_STR_EQ(n->name,"AddExp"))
    {
        auto node = new ASTAddExp();
        if(n->children_num == 1)
        {
            auto mul_exp = static_cast<ASTMulExp *>(transform_node_iter(n->children[0]));
            node->mul_exp = std::shared_ptr<ASTMulExp>(mul_exp);
        }
        else if(n->children_num == 3)
        {
            auto add_exp = static_cast<ASTAddExp *>(transform_node_iter(n->children[0]));
            node->add_exp = std::shared_ptr<ASTAddExp>(add_exp);
            auto mul_exp = static_cast<ASTMulExp *>(transform_node_iter(n->children[2]));
            node->mul_exp = std::shared_ptr<ASTMulExp>(mul_exp);
            if(_STR_EQ(n->children[1]->name , "+"))
            {
                node->op = OP_PLUS;
            }
            else if(_STR_EQ(n->children[1]->name,"-"))
            {
                node->op = OP_MINUS;
            }
            else{
                _AST_NODE_ERROR_
            }
        }
        else{
            _AST_NODE_ERROR_
        }
        return node;
    }
    /*
    LOrExp : LAndExp { $$ = node("LOrExp", 1, $1); }
        | LOrExp OR LAndExp { $$ = node("LOrExp", 3, $1, $2, $3); }
        ;
    */
    else if(_STR_EQ(n->name,"LOrExp"))
    {
        auto node = new ASTLOrExp();
        if(n->children_num == 1)
        {
            //printf("l_or_exp    1\n");
            auto l_and_exp = static_cast<ASTLAndExp *>(transform_node_iter(n->children[0]));
            node->l_and_exp = std::shared_ptr<ASTLAndExp>(l_and_exp);
          //  node->l_or_exp = nullptr;
        }
        else if(n->children_num == 3)
        {
            ////printf("l_or_exp    3\n");
            auto l_or_exp = static_cast<ASTLOrExp *>(transform_node_iter(n->children[0]));
            node->l_or_exp = std::shared_ptr<ASTLOrExp>(l_or_exp);
            node->op = OP_OR;
            auto l_and_exp = static_cast<ASTLAndExp *>(transform_node_iter(n->children[2]));
            node->l_and_exp = std::shared_ptr<ASTLAndExp>(l_and_exp);
            
        }
        else{
            _AST_NODE_ERROR_
        }
        return node;
    }
    /*
    LAndExp : EqExp { $$ = node("LAndExp", 1, $1); }
        | LAndExp AND EqExp { $$ = node("LAndExp", 3, $1, $2, $3); }
        ;
    */
    else if(_STR_EQ(n->name,"LAndExp"))
    {
        auto node = new ASTLAndExp();
        if(n->children_num == 1)
        {
            ////printf("eq_exp   1\n");
            auto eq_exp = static_cast<ASTEqExp *>(transform_node_iter(n->children[0]));
            node->eq_exp = std::shared_ptr<ASTEqExp>(eq_exp);
        //    node->l_and_exp = nullptr;
        }
        else if(n->children_num == 3)
        {
            auto l_and_exp = static_cast<ASTLAndExp *>(transform_node_iter(n->children[0]));
            node->l_and_exp = std::shared_ptr<ASTLAndExp>(l_and_exp);
            node->op = OP_AND;
            auto eq_exp = static_cast<ASTEqExp *>(transform_node_iter(n->children[2]));
            node->eq_exp = std::shared_ptr<ASTEqExp>(eq_exp);
            
        }
        else{
            _AST_NODE_ERROR_
        }
        return node;
    }
    /*
    RelExp : AddExp { $$ = node("RelExp", 1, $1); }
        | RelExp LT AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        | RelExp GT AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        | RelExp LE AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        | RelExp GE AddExp { $$ = node("RelExp", 3, $1, $2, $3); }
        ;
    */
    else if(_STR_EQ(n->name,"RelExp"))
    {
        auto node = new ASTRelExp();
        if(n->children_num == 1)
        {
            //printf("relexp    v\n");
            auto add_exp = static_cast<ASTAddExp *>(transform_node_iter(n->children[0]));
            node->add_exp = std::shared_ptr<ASTAddExp>(add_exp);
        }
        else if(n->children_num == 3)
        {
            //printf("relexp\n");
            auto rel_exp = static_cast<ASTRelExp *>(transform_node_iter(n->children[0]));
            node->rel_exp = std::shared_ptr<ASTRelExp>(rel_exp);
            if(_STR_EQ(n->children[1]->name ,"<"))
            {
                node->op = OP_LT;
            }
            else if(_STR_EQ(n->children[1]->name,">"))
            {
                node->op = OP_GT;
            }
            else if(_STR_EQ(n->children[1]->name , "<="))
            {
                node->op = OP_LE;
            }
            else if(_STR_EQ(n->children[1]->name , ">="))
            {
                node->op = OP_GE;
            }
            else{
                _AST_NODE_ERROR_
            }
            auto add_exp = static_cast<ASTAddExp *>(transform_node_iter(n->children[2]));
            node->add_exp = std::shared_ptr<ASTAddExp>(add_exp);
           
        }
        else{
            _AST_NODE_ERROR_
        }
        return node;
    }
    /*
    EqExp : RelExp { $$ = node("EqExp", 1, $1); }
        | EqExp EQ RelExp { $$ = node("EqExp", 3, $1, $2, $3); }
        | EqExp NEQ RelExp { $$ = node("EqExp", 3, $1, $2, $3); }
        ;
    */
    else if(_STR_EQ(n->name,"EqExp"))
    {
        auto node = new ASTEqExp();
        if(n->children_num == 1)
        {
            auto rel_exp = static_cast<ASTRelExp *>(transform_node_iter(n->children[0]));
            node->rel_exp = std::shared_ptr<ASTRelExp>(rel_exp);
        }
        else
        {
            auto eq_exp =   static_cast<ASTEqExp *>(transform_node_iter(n->children[0]));
            node->eq_exp = std::shared_ptr<ASTEqExp>(eq_exp);
            if(_STR_EQ(n->children[1]->name,"=="))
            {
                //printf("eq\n");
                node->op = OP_EQ;
            }
            else
            {
                node->op = OP_NEQ;
            }
            auto rel_exp = static_cast<ASTRelExp *>(transform_node_iter(n->children[2]));
            node->rel_exp = std::shared_ptr<ASTRelExp>(rel_exp);
        }
        return node;
    }
    /*
    ConstExp : AddExp { $$ = node("ConstExp", 1, $1); }
        ;
    */
    else if(_STR_EQ(n->name,"ConstExp"))
    {
        auto node = new ASTConstExp();
        auto add_exp = static_cast<ASTAddExp *>(transform_node_iter(n->children[0]));
        node->add_exp = std::shared_ptr<ASTAddExp>(add_exp);
        return node;
    }
    else{
        std::cerr << "[ast]: transform failure!" << std::endl;
        std::abort();
    }
}

Value *ASTCompUnit::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTDecl::accept(ASTVisitor &visitor) {
    if(this->const_decl != nullptr){
        return this->const_decl->accept(visitor);
    }
    else if(this->var_decl != nullptr){
        return this->var_decl->accept(visitor);
    }
    else if(this->func_def != nullptr){
        return this->func_def->accept(visitor);
    }
    else{
    }
}
Value *ASTConstDecl::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTConstDef::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTConstInitVal::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTVarDecl::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTVarDef::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTInitVal::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTFuncDef::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTFuncFParam::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
//Value *ASTFuncArrayIdent::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTBlock::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTBlockItem::accept(ASTVisitor &visitor) {
    if(this->stmt != nullptr){
        return this->stmt->accept(visitor);
    }
    else if(this->const_decl != nullptr){
        return this->const_decl->accept(visitor);
    }
    else if(this->var_decl != nullptr){
        return this->var_decl->accept(visitor);
    }
    else{
    }
}
Value *ASTStmt::accept(ASTVisitor &visitor) {
    if(this->assign_stmt != nullptr){
        return this->assign_stmt->accept(visitor);
    }
    else if(this->exp != nullptr){
        return this->exp->accept(visitor);
    }
    else if(this->block != nullptr){
        return this->block->accept(visitor);
    }
    else if(this->selection_stmt != nullptr){
        return this->selection_stmt->accept(visitor);
    }
    else if(this->iteration_stmt != nullptr){
        return this->iteration_stmt->accept(visitor);
    }
    else if(this->return_stmt != nullptr){
        return this->return_stmt->accept(visitor);
    }
    else if(this->break_stmt != nullptr){
        return this->break_stmt->accept(visitor);
    }
    else if(this->continue_stmt != nullptr){
        return this->continue_stmt->accept(visitor);
    }
    else{
        return nullptr;
    }
}
Value *ASTExp::accept(ASTVisitor &visitor) {return this->add_exp->accept(visitor);}
Value *ASTCond::accept(ASTVisitor &visitor) {return this->l_or_exp->accept(visitor);}
Value *ASTLVal::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTPrimaryExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTNumber::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTUnaryExp::accept(ASTVisitor &visitor) { return visitor.visit(*this);}
Value *ASTMulExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTAddExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTLOrExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTLAndExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTRelExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTConstExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTSelectionStmt::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTIterationStmt::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTReturnStmt::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTBreakStmt::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTContinueStmt::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTAssignStmt::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTCall::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTEqExp::accept(ASTVisitor &visitor) {return visitor.visit(*this);}
Value *ASTFuncArrayIdent::accept(ASTVisitor &visitor) {return nullptr;}
Value *ASTArrayIdent::accept(ASTVisitor &visitor) {return nullptr;}
Value *ASTConstArrayIdent::accept(ASTVisitor &visitor) {return nullptr;}

#define _DEBUG_PRINT_N_(N)                                                     \
    { std::cout << std::string(N, '-'); }

Value *ASTPrinter::visit(ASTCompUnit &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "CompUnit" << std::endl;
    add_depth();
    for (auto &decl : node.declarations) {
        decl->accept(*this);
    } 
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTDecl &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "Decl" << std::endl;
    if(node.const_decl != nullptr){
        node.const_decl->accept(*this);
    }
    else if(node.var_decl != nullptr){
        node.var_decl->accept(*this);
    }
    else if(node.func_def != nullptr){
        node.func_def->accept(*this);
    }
    return nullptr;
}

Value *ASTPrinter::visit(ASTConstDecl &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstDecl" << std::endl;
    add_depth();
    for (auto &const_def : node.const_defs) {
        const_def->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTConstDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstDef   :" << node.id << std::endl;
    add_depth();
    if(node.const_array_ident != nullptr){ 
        for(auto &const_exps : node.const_array_ident->const_exps)
        {
            const_exps->accept(*this);
        }
    }
    node.const_init_val->accept(*this);
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTConstInitVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstInitVal" << std::endl;
    add_depth();
    if(node.const_exp != nullptr){
        node.const_exp->accept(*this);
    }
    else{
        for(auto &const_init_val : node.const_init_vals)
        {
            const_init_val->accept(*this);
        }
    }
    return nullptr;
}

Value *ASTPrinter::visit(ASTVarDecl &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "VarDecl" << std::endl;
    add_depth();
    for (auto &var_def : node.var_defs) {
        var_def->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTVarDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "VarDef   :" << node.id << std::endl;
    add_depth();
    if(node.array_ident != nullptr){
        for(auto &exp : node.array_ident->const_exps){
            exp->accept(*this);
        }
    }
    if(node.init_val != nullptr){
        node.init_val->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTInitVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "InitVal" << std::endl;
    add_depth();
    if(node.exp != nullptr){
        node.exp->accept(*this);
    }
    else{
        for(auto &init_val : node.init_vals){
            ////printf("init_val\n");
            init_val->accept(*this);
        }
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTFuncDef &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "FuncDef   :" << node.id << std::endl;
    add_depth();
    for(auto &func_fparam : node.func_fparams){
        func_fparam->accept(*this);
    }
    node.block->accept(*this);
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTFuncFParam &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "FuncFParam   :" << node.id << "  isarray ?   " << node.isarray << std::endl;
    add_depth();
    if(node.isarray){
        for(auto &exp : node.array_ident->exps){
            exp->accept(*this);
        }
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTBlock &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Block" << std::endl;
    add_depth();
    for(auto &block_item : node.block_items){
        block_item->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTBlockItem &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "BlockItem" << std::endl;
    add_depth();
    if(node.stmt != nullptr){
        node.stmt->accept(*this);
    }
    else if(node.const_decl != nullptr){
        node.const_decl->accept(*this);
    }
    else if(node.var_decl != nullptr){
        node.var_decl->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Stmt" << std::endl;
    add_depth();
    if(node.assign_stmt != nullptr){
        node.assign_stmt->accept(*this);
    }
    else if(node.exp != nullptr){
        node.exp->accept(*this);
    }
    else if(node.block != nullptr){
        node.block->accept(*this);
    }
    else if(node.selection_stmt != nullptr){
        node.selection_stmt->accept(*this);
    }
    else if(node.iteration_stmt != nullptr){
        node.iteration_stmt->accept(*this);
    }
    else if(node.return_stmt != nullptr){
        node.return_stmt->accept(*this);
    }
    else if(node.break_stmt != nullptr){
        node.break_stmt->accept(*this);
    }
    else if(node.continue_stmt != nullptr){
        node.continue_stmt->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Exp" << std::endl;
    add_depth();
    node.add_exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTAssignStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "AssignStmt" << std::endl;
    add_depth();
    node.l_val->accept(*this);
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTSelectionStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "SelectionStmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.if_stmt->accept(*this);
    if(node.else_stmt != nullptr){
        node.else_stmt->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTIterationStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "IterationStmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.stmt->accept(*this);
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTReturnStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ReturnStmt" << std::endl;
    add_depth();
    if(node.exp != nullptr){
        node.exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTBreakStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "BreakStmt" << std::endl;
    return nullptr;
}

Value *ASTPrinter::visit(ASTContinueStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ContinueStmt" << std::endl;
    return nullptr;
}

Value *ASTPrinter::visit(ASTAddExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "AddExp   :" <<node.op<< std::endl;
    add_depth();
    if(node.add_exp != nullptr){
        node.add_exp->accept(*this);
    }
    if(node.mul_exp != nullptr){
        node.mul_exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTMulExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "MulExp   :" <<node.op<< std::endl;
    add_depth();
    if(node.mul_exp != nullptr){
        node.mul_exp->accept(*this);
    }
    if(node.unary_exp != nullptr){
        node.unary_exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTUnaryExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "UnaryExp   :" << std::endl;
    add_depth();
    if(node.primary_exp != nullptr){
        node.primary_exp->accept(*this);
    }
    else
    if(node.call != nullptr){
        node.call->accept(*this);
    }
    else 
    if(node.unary_exp != nullptr){
        std::cout << "op: " << node.op << std::endl;
        node.unary_exp->accept(*this);
    }
   
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTPrimaryExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "PrimaryExp  :" << std::endl;
    add_depth();
    if(node.exp != nullptr){
        node.exp->accept(*this);
    }
    if(node.l_val != nullptr){
        node.l_val->accept(*this);
    }
    if(node.number != nullptr){
        node.number->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTLVal &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "LVal   :" << node.id << std::endl;
    add_depth();
    if(node.array_ident != nullptr){
        for(auto &exp : node.array_ident->exps){
            exp->accept(*this);
        }
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTNumber &node)
{
    _DEBUG_PRINT_N_(depth);
    if(node.type == TYPE_INT){
        //printf("0x%x int hex value\n", node.int_val);
        std::cout << "Number   :" << node.int_val << std::endl;
    }
    else if(node.type == TYPE_FLOAT){
        //printf("%a float hex value\n", node.float_val);
        std::cout << "Number   :" << node.float_val << std::endl;
    }
    return nullptr;
}

Value *ASTPrinter::visit(ASTCond &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Cond  :" << std::endl;
    add_depth();
    node.l_or_exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTLOrExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "LOrExp  :" <<node.op<< std::endl;
    add_depth();
    if(node.l_or_exp != nullptr){
        node.l_or_exp->accept(*this);
    }
    if(node.l_and_exp != nullptr){
        node.l_and_exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTLAndExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "LAndExp  :" <<node.op<< std::endl;
    add_depth();
    if(node.l_and_exp != nullptr){
        node.l_and_exp->accept(*this);
    }
    if(node.eq_exp != nullptr){
        node.eq_exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTRelExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "RelExp  :" <<node.op<< std::endl;
    add_depth();
    if(node.rel_exp != nullptr){
        node.rel_exp->accept(*this);
    }
    if(node.add_exp != nullptr){
        node.add_exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}


Value *ASTPrinter::visit(ASTEqExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "EqExp  :" <<node.op<< std::endl;
    add_depth();
    if(node.eq_exp != nullptr){
        node.eq_exp->accept(*this);
    }
    if(node.rel_exp != nullptr){
        node.rel_exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTConstExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstExp  :" << std::endl;
    add_depth();
    node.add_exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value *ASTPrinter::visit(ASTCall &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Call  :" << node.id << std::endl;
    add_depth();
    for(auto &exp : node.exps){
        exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}