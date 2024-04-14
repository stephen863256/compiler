#include "SysY_builder.hpp"
#include "Constant.hpp"
#include "ast.hpp"
#include "logging.hpp"
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int if_count = 0;
int while_count = 0;
//bool need_const = false;
#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())
#define __Array_Padding_IF__(bool_exp, action)                   \
                    while(bool_exp) {                            \
                        if(cur_type == FLOAT_T)                  \
                            init_val.push_back(CONST_FP(0));     \
                        else                                     \
                            init_val.push_back(CONST_INT(0));    \
                        action;                                  \
                    }   
//参考https://gitlab.eduxiji.net/educg-group-17291-1894922/202310358201709-501/-/blob/main/src/irbuilder/sysY_irbuilder.cc

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

std::map<int, Value *> init_val_map;
bool global_init = false;
int cur_depth;
int cur_pos ;
std::vector<int> array_bound;
std::vector<int> array_sizes;
std::vector<Constant *> init_val;
Type *cur_type = nullptr;

 Value* SysYBuilder::visit(ASTCompUnit &node) {
    VOID_T = module->get_void_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();

    Value *ret_val = nullptr;
    for (auto &decl : node.declarations) {
        ret_val = decl->accept(*this);
    }
    return ret_val;
 }


 Value* SysYBuilder::visit(ASTConstDecl &node) {
     if(node.type == TYPE_INT)
    {
        cur_type = INT32_T;
    }
    else if(node.type == TYPE_FLOAT)
    {
        cur_type = FLOAT_T;
    }
    for (auto &const_def : node.const_defs) {
        const_def->accept(*this);
    }
    return nullptr;
 }

 Value* SysYBuilder::visit(ASTConstDef &node) {
    //////std::cout<< "const init" << std::endl;
    if(node.const_array_ident == nullptr){
        node.const_init_val->accept(*this);
        auto val = context.cur_val;
     //   LOG_DEBUG <<  "const_def " << node.id << " " ;
        if(cur_type == INT32_T && val->get_type()->is_float_type())
        {
            val = CONST_INT((int)(dynamic_cast<ConstantFP *>(val)->get_value()));
        }
        else if(cur_type == FLOAT_T && val->get_type()->is_int32_type())
        {
            val = CONST_FP((float)(dynamic_cast<ConstantInt *>(val)->get_value()));
        }
        scope.push(node.id,val);
    }
    else{ 
        array_sizes.clear();
        array_bound.clear();
        for(auto &size : node.const_array_ident->const_exps){
            size->accept(*this);
            auto array_value = dynamic_cast<ConstantInt *>(context.cur_val)->get_value();
            array_bound.push_back(array_value);
        }     
        int total_size = 1;
        for(auto iter = array_bound.rbegin(); iter != array_bound.rend(); iter++){
            array_sizes.insert(array_sizes.begin(),total_size);
            total_size *= (*iter);
        }
        array_sizes.insert(array_sizes.begin(),total_size);
        //////std::cout<< "total_size "<<total_size << std::endl;
        auto array_type = ArrayType::get(cur_type,total_size);
        cur_depth = 0;
        cur_pos = 0;
        init_val.clear();
       // init_val_map.clear();
        node.const_init_val->accept(*this);
        //////std::cout<< "init_val size" << init_val.size() << std::endl;
       //for(auto &val : init_val){
       //     ////std::cout<< dynamic_cast<ConstantInt*>(val)->get_value() << std::endl;
       // }
        auto initializer = ConstantArray::get(array_type,init_val);
        ////std::cout<< "const init" << std::endl;
        if(scope.in_global())
        {
            auto global_var = GlobalVariable::create(node.id,module.get(),array_type,true,initializer);
            scope.push(node.id,global_var);
            scope.push_params(node.id,array_sizes);
            scope.push_const(node.id,initializer);
        }
        else
        {
            auto var = builder->create_alloca(array_type);
            for(int i = 0;i < total_size;i ++)
            {
                auto ptr = builder->create_gep(var,{CONST_INT(0),CONST_INT(i)});
                if(init_val_map[i])
                {
                    builder->create_store(init_val_map[i],ptr);
                }
                else
                {
                    if(cur_type == FLOAT_T)
                    {
                        builder->create_store(CONST_FP(0.0),ptr);
                    }
                    else
                    {
                        builder->create_store(CONST_INT(0),ptr);
                    }
                }
            }
            scope.push(node.id,var);
            scope.push_params(node.id,array_sizes);
            scope.push_const(node.id,initializer);
        }
    }
    return nullptr;
 }

 Value* SysYBuilder::visit(ASTConstInitVal &node) {
    ////std::cout<< "const init val    cur_depth" << cur_depth <<std::endl;
    if(node.const_init_vals.empty() && node.const_exp) {
        node.const_exp->accept(*this);
        auto tmp_int32_val = dynamic_cast<ConstantInt*>(context.cur_val);
        auto tmp_float_val = dynamic_cast<ConstantFP*>(context.cur_val);
        if (cur_type == INT32_T && tmp_float_val != nullptr) {
            context.cur_val = CONST_INT(int(tmp_float_val->get_value()));
        } else if(cur_type == FLOAT_T && tmp_int32_val != nullptr) {
            context.cur_val = CONST_FP(float(tmp_int32_val->get_value()));
        }
        init_val_map[cur_pos] = context.cur_val;
        init_val.push_back(static_cast<Constant*>(context.cur_val));
        cur_pos++;
    } else {
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos % array_sizes[cur_depth] != 0, cur_pos++);
        }
        int cur_start_pos = cur_pos;
        for(const auto& const_init_val : node.const_init_vals) {
            cur_depth++;
            const_init_val->accept(*this);
            cur_depth--;
        }
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos < cur_start_pos + array_sizes[cur_depth], cur_pos++);
        } else {
            __Array_Padding_IF__(cur_pos < array_sizes[0], cur_pos++);
        }
    }


    return nullptr;      
 }

 Value *SysYBuilder::visit(ASTVarDecl &node) {
    if(node.type == TYPE_INT )
    {
        cur_type = INT32_T;
    }
    else if(node.type == TYPE_FLOAT)
    {
        cur_type = FLOAT_T;
    }
    for (auto &var_def : node.var_defs) {
        var_def->accept(*this);
    }
    return nullptr;
 }

 Value *SysYBuilder::visit(ASTVarDef &node) {
    if(node.array_ident == nullptr)
    {
        if(node.init_val != nullptr)
        {
            if(scope.in_global())
            {
                node.init_val->accept(*this);
                if(cur_type == INT32_T)
                {
                    auto val = dynamic_cast<ConstantInt *>(context.cur_val);
                    auto global_var = GlobalVariable::create(node.id,module.get(),val->get_type(),false,val);
                     scope.push(node.id,global_var);
                }
                else if(cur_type == FLOAT_T)
                {
                    auto val = dynamic_cast<ConstantFP *>(context.cur_val);
                    auto global_var = GlobalVariable::create(node.id,module.get(),val->get_type(),false,val);
                     scope.push(node.id,global_var);
                }
            }
            else
            {
                auto var = builder->create_alloca(cur_type);
                node.init_val->accept(*this);
                auto val = context.cur_val;
                if(cur_type == INT32_T && val->get_type()->is_float_type())
                {
                    val = builder->create_fptosi(val,INT32_T);
                }
                else if(cur_type == FLOAT_T && val->get_type()->is_int32_type())
                {
                    val = builder->create_sitofp(val,FLOAT_T);
                }
                builder->create_store(val,var);
                ////std::cout<< node.id <<"local var"  <<std::endl;
                scope.push(node.id,var);
            }  
        }
        else
        {
            if(scope.in_global())
            {
              //  LOG_DEBUG << "var_def " << node.id << " ";
                auto initializer = ConstantZero::get(cur_type,module.get());
                auto global_var = GlobalVariable::create(node.id,module.get(),cur_type,false,initializer);
                scope.push(node.id,global_var);
            }
            else
            {
                  ////std::cout<< node.id <<" local var"  <<std::endl;
                auto var = builder->create_alloca(cur_type);
                scope.push(node.id,var);
            }
        }
    }
    else
    {
        ////std::cout<< "array init     \n" << std::endl;
        ////std::cout<< node.id << std::endl;
        array_sizes.clear();
        array_bound.clear();
        
        for(auto &size : node.array_ident->const_exps){
            size->accept(*this);
            auto array_value = dynamic_cast<ConstantInt *>(context.cur_val)->get_value();
            array_bound.push_back(array_value);
        }     
        int total_size = 1;
        for(auto iter = array_bound.rbegin(); iter != array_bound.rend(); iter++)
        {
            array_sizes.insert(array_sizes.begin(),total_size);
            total_size *= (*iter);
        }
        array_sizes.insert(array_sizes.begin(), total_size);

       // //std::cout<< "total_size "<<total_size << std::endl;
        auto array_type = ArrayType::get(cur_type,total_size);
        cur_depth = 0;
        cur_pos = 0;
        init_val.clear();
        init_val_map.clear();
       // node.init_val->accept(*this);
        if(scope.in_global())
        {
            if(node.init_val)
            {
                if(node.init_val->init_vals.empty())
                {
                     auto initializer = ConstantZero::get(array_type,module.get());
                    auto global_var = GlobalVariable::create(node.id,module.get(),array_type,false,initializer);
                    scope.push(node.id,global_var);
                    scope.push_params(node.id,array_sizes);
                    return nullptr;
                }
                ////std::cout<< "init_val size    " << init_val.size() << std::endl;
                node.init_val->accept(*this);
                init_val.resize(total_size);
              //  //std::cout<< "init_val size    " << init_val.size() << std::endl;
               // for(auto &val : init_val){
             //       //std::cout<< dynamic_cast<ConstantInt*>(val)->get_value() << std::endl;
                //}
                ////std::cout<< array_sizes.size() << " array_sizes" << std::endl;
                //for(auto array_size : array_sizes){
                //    //std::cout<< array_size << " array_size" << std::endl;
                //}
                auto initializer = ConstantArray::get(array_type,init_val);
                auto global_var = GlobalVariable::create(node.id,module.get(),array_type,false,initializer);
                scope.push(node.id,global_var);
                scope.push_params(node.id,array_sizes);
            }
            else
            {
               // LOG_DEBUG << "var_def " << node.id << " ";
                auto initializer = ConstantZero::get(array_type,module.get());
                auto global_var = GlobalVariable::create(node.id,module.get(),array_type,false,initializer);
                scope.push(node.id,global_var);
                scope.push_params(node.id,array_sizes);
            }
        }
        else
        {
            auto var = builder->create_alloca(array_type);
            if(node.init_val)
            {
                node.init_val->accept(*this);
                ////std::cout<< "local var " << std::endl;
               // //std::cout<< "init_val size " << init_val.size() << std::endl;
              //  for(auto &val : init_val){
                    ////std::cout<< dynamic_cast<ConstantInt*>(val)->get_value() << std::endl;
                //}
                for(int i = 0;i < total_size;i ++)
                {
                    auto ptr = builder->create_gep(var,{CONST_INT(0),CONST_INT(i)});
                    if(init_val_map[i])
                    {
                        builder->create_store(init_val_map[i],ptr);
                    }
                    else
                    {
                        if(cur_type == FLOAT_T)
                        {
                            builder->create_store(CONST_FP(0.0),ptr);
                        }
                        else
                        {
                            builder->create_store(CONST_INT(0),ptr);
                        }       
                    }
                }
            }
            scope.push(node.id,var);
            scope.push_params(node.id,array_sizes);
        }
    }
    return nullptr;
 }

 Value *SysYBuilder::visit(ASTInitVal &node){
     if(node.init_vals.empty() && node.exp) {
        node.exp->accept(*this);
        auto tmp_int32_val = dynamic_cast<ConstantInt*>(context.cur_val);
        auto tmp_float_val = dynamic_cast<ConstantFP*>(context.cur_val);
        if (cur_type == INT32_T && tmp_float_val != nullptr) {
            context.cur_val = CONST_INT(int(tmp_float_val->get_value()));
        } else if(cur_type == INT32_T && context.cur_val->get_type()->is_float_type()) {
            context.cur_val = builder->create_fptosi(context.cur_val,INT32_T);
        }
        else if(cur_type == FLOAT_T && tmp_int32_val != nullptr) {
            context.cur_val = CONST_FP(float(tmp_int32_val->get_value()));
        }
        else if(cur_type == FLOAT_T && context.cur_val->get_type()->is_int32_type()) {
            context.cur_val = builder->create_sitofp(context.cur_val,FLOAT_T);
        }
        init_val_map[cur_pos] = context.cur_val;
        init_val.push_back(dynamic_cast<Constant*>(context.cur_val));
     //   //std::cout<< cur_pos << " cur_pos   " << init_val.size() << std::endl;
        cur_pos++;
    } else {
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos % array_sizes[cur_depth] != 0, cur_pos++);
        }
        int cur_start_pos = cur_pos;
        for(const auto& init_val : node.init_vals) {
            cur_depth++;
          //  //std::cout<< "cur_depth   " << cur_depth << std::endl;
            init_val->accept(*this);
            cur_depth--;
        }
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos < cur_start_pos + array_sizes[cur_depth], cur_pos++);
        } else {
            __Array_Padding_IF__(cur_pos < array_sizes[0], cur_pos++);
        }
    }

    return nullptr;
 }

 Value *SysYBuilder::visit(ASTFuncDef &node) {
    FunctionType *func_type;
    Type *ret_type;
    if(node.type == TYPE_INT)
    {
        ret_type = INT32_T;
    }
    else if(node.type == TYPE_FLOAT)
    {
        ret_type = FLOAT_T;
    }
    else
    {
        ret_type = VOID_T;
    }
    std::vector<Type *> param_types;
    for(auto &param : node.func_fparams)
    {
        if(param->isarray)
        {
            if(param->type == TYPE_INT)
            {
                param_types.push_back(INT32PTR_T);
            }
            else if(param->type == TYPE_FLOAT)
            {
                param_types.push_back(FLOATPTR_T);
            }
        }
        else
        {
            if(param->type == TYPE_INT)
            {
                param_types.push_back(INT32_T);
            }
            else if(param->type == TYPE_FLOAT)
            {
                param_types.push_back(FLOAT_T);
            }
        }
    }
    func_type = FunctionType::get(ret_type,param_types);
    auto func = Function::create(func_type,node.id,module.get());
    //LOG_DEBUG << "func_def " << node.id << " ";
    scope.push_func(node.id,func);
    ////std::cout<< "func_def   " <<node.id<< std::endl;
    context.func = func;
    auto bb = BasicBlock::create(module.get(),"entry",func);
    builder->set_insert_point(bb);
    scope.enter();
    std::vector<Value *> args;
    for(auto &arg : func->get_args())
    {
        args.push_back(&arg);
    }

    for(int i = 0;i < node.func_fparams.size();i++)
    {
        Type *type = param_types[i];
        auto alloca = builder->create_alloca(type);
        builder->create_store(args[i],alloca);
        scope.push(node.func_fparams[i]->id,alloca);
        array_bound.clear();
        array_sizes.clear();
        if(node.func_fparams[i]->isarray)
        {
            array_bound.push_back(1);
            for (auto array_param : node.func_fparams[i]->array_ident->exps) {
                array_param->accept(*this);
                auto bound = dynamic_cast<ConstantInt *>(context.cur_val)->get_value();
                array_bound.push_back(bound);
            }
            int total_size = 1;
            for (auto iter = array_bound.rbegin(); iter != array_bound.rend(); iter++) {
                array_sizes.insert(array_sizes.begin(), total_size);
                total_size *= (*iter);
            }
            array_sizes.insert(array_sizes.begin(), total_size);
            scope.push_params(node.func_fparams[i]->id,array_sizes);
        }
    }
    node.block->accept(*this);
    if(not builder->get_insert_block()->is_terminated())
    {
        if(context.func->get_return_type()->is_void_type())
        {
            builder->create_void_ret();
        }
        else if(context.func->get_return_type()->is_float_type())
        {
            builder->create_ret(CONST_FP(0.));
        }
        else
        {
            builder->create_ret(CONST_INT(0));
        }
    }
    scope.exit();
    return nullptr;
 }


bool pre_enter_scope = false;
Value *SysYBuilder::visit(ASTBlock &node) {
    bool need_exit_scope = !pre_enter_scope;
    if(pre_enter_scope)
    {
        pre_enter_scope = false;
    } else{
        scope.enter();
    }

    for(auto &block_item : node.block_items)
    {
        if(block_item->stmt != nullptr)
        {
            block_item->stmt->accept(*this);
            if(builder->get_insert_block()->is_terminated())
            {
                break;
            }
        }
        else if(block_item->const_decl != nullptr)
        {
            block_item->const_decl->accept(*this);
        }
        else if(block_item->var_decl != nullptr)
        {
            block_item->var_decl->accept(*this);
        }
    }

    if(need_exit_scope)
    {
        scope.exit();
    }
    return nullptr;
}

bool need_lvalue = false;
Value *SysYBuilder::visit(ASTAssignStmt &node) {
    need_lvalue = true;
    node.l_val->accept(*this); 
    auto l_val = context.cur_val;
    node.exp->accept(*this);
    auto exp = context.cur_val;
   // //std::cout<< l_val->get_type()->is_pointer_type() << "check" << std::endl;
    //std::cout<< l_val->get_type()->get_pointer_element_type()->is_integer_type() << "check" << std::endl; 
    if(l_val->get_type()->get_pointer_element_type() != exp->get_type())
    {
        if(exp->get_type()->is_float_type())
        {
            auto const_val = dynamic_cast<ConstantFP *>(exp);
            if(const_val)
            {
                exp = CONST_INT((int)(const_val->get_value()));
            }
            else
            { 
                //std::cout<< "sitofp" << std::endl;
                exp = builder->create_fptosi(exp,INT32_T);
            }
        }
        else
        {
            auto const_val = dynamic_cast<ConstantInt *>(exp);
            if(const_val)
            {
                exp = CONST_FP((float)const_val->get_value());
            }
            else
            {
                exp = builder->create_sitofp(exp,FLOAT_T);
            }
        }
    }

    builder->create_store(exp,l_val);
    context.cur_val = exp;
    return nullptr;
}
struct  BB{
    BasicBlock *trueBB;
    BasicBlock *falseBB;
}
;
std::vector<BB> loop_stack;

std::vector<BasicBlock *> true_stack;
std::vector<BasicBlock *> false_stack;
Value *SysYBuilder::visit(ASTSelectionStmt &node) {
    auto trueBB = BasicBlock::create(module.get(),"trueBB"+std::to_string(if_count),context.func);
    auto falseBB = BasicBlock::create(module.get(),"falseBB"+std::to_string(if_count),context.func);
    auto mergeBB = BasicBlock::create(module.get(),"mergeBB"+std::to_string(if_count),context.func);
    if_count ++;
    true_stack.push_back(trueBB);
    false_stack.push_back(falseBB);
    node.cond->accept(*this);
    true_stack.pop_back();
    false_stack.pop_back();
    if(context.cur_val->get_type()->is_float_type())
    {
        //auto const_value = dynamic_cast<ConstantFP *>(context.cur_val);
        //if(const_value)
        //{
        //    context.cur_val = CONST_INT((int)(const_value->get_value()));
       // }
        //else
        //{
            context.cur_val = builder->create_fcmp_ne(context.cur_val,CONST_FP(0.));
       // }
    }
    else
    {
        //auto const_value = dynamic_cast<ConstantInt *>(context.cur_val);
        //if(const_value)
        //{
          //  context.cur_val = CONST_INT(const_value->get_value());
       // }
        //else
        //{
            context.cur_val = builder->create_icmp_ne(context.cur_val,CONST_INT(0));
       // }
    }
    builder->create_cond_br(context.cur_val,trueBB,falseBB);
    builder->set_insert_point(trueBB);
    scope.enter();
    node.if_stmt->accept(*this);
    scope.exit();
    if(not builder->get_insert_block()->is_terminated())
    {
        builder->create_br(mergeBB);
    }

    builder->set_insert_point(falseBB);
    if(node.else_stmt != nullptr)
    {
        scope.enter();
        node.else_stmt->accept(*this);
        scope.exit();
        if(not builder->get_insert_block()->is_terminated())
        {
            builder->create_br(mergeBB);
        }
    }
    else
        builder->create_br(mergeBB);

    builder->set_insert_point(mergeBB);


    return nullptr;
}


Value *SysYBuilder::visit(ASTIterationStmt &node) {
    auto testBB = BasicBlock::create(module.get(),"testBB"+std::to_string(while_count),context.func);
    auto condBB = BasicBlock::create(module.get(),"condBB"+std::to_string(while_count),context.func);
    auto afterBB = BasicBlock::create(module.get(),"afterBB"+std::to_string(while_count),context.func);
    while_count ++;
    builder->create_br(testBB);
    builder->set_insert_point(testBB);
    loop_stack.push_back({testBB,afterBB}); 

    true_stack.push_back(condBB);
    false_stack.push_back(afterBB);

    node.cond->accept(*this);

    true_stack.pop_back();
    false_stack.pop_back();
    if(context.cur_val->get_type()->is_float_type())
    {
       // auto const_value = dynamic_cast<ConstantFP *>(context.cur_val);
       // if(const_value)
       // {
      //      context.cur_val = CONST_INT((int)(const_value->get_value()));
       // }
       // else
      //  {
            context.cur_val = builder->create_fcmp_ne(context.cur_val,CONST_FP(0.));
       // }

    }
    else
    {
       // auto const_value = dynamic_cast<ConstantInt *>(context.cur_val);
       // if(const_value)
       // {
       //     context.cur_val = CONST_INT(const_value->get_value());
       // }
      //  else
      //  {
            context.cur_val = builder->create_icmp_ne(context.cur_val,CONST_INT(0));
     //   }
    }
    builder->create_cond_br(context.cur_val,condBB,afterBB);

    builder->set_insert_point(condBB);
    scope.enter();
    node.stmt->accept(*this);
    scope.exit();
    if(not builder->get_insert_block()->is_terminated())
    {
        builder->create_br(testBB);
    }

    builder->set_insert_point(afterBB);
    loop_stack.pop_back();
    return nullptr;
}

Value *SysYBuilder::visit(ASTReturnStmt &node){
    ////std::cout<< "return stmt" << std::endl;
   // LOG_DEBUG << "return_stmt ";
    if(node.exp == nullptr)
    {
        builder->create_void_ret();
    }
    else
    {
        node.exp->accept(*this);
        if(context.func->get_function_type()->get_return_type() != context.cur_val->get_type())
        {
            if(context.func->get_function_type()->get_return_type()->is_float_type())
            {
                context.cur_val = builder->create_sitofp(context.cur_val,FLOAT_T);   
            }
            else
            {
                context.cur_val = builder->create_fptosi(context.cur_val,INT32_T);
            }
        }
        builder->create_ret(context.cur_val);
    }

    return nullptr;
}

Value *SysYBuilder::visit(ASTBreakStmt &node){
    builder->create_br(loop_stack.back().falseBB);
    return nullptr;
}

Value *SysYBuilder::visit(ASTContinueStmt &node){
    builder->create_br(loop_stack.back().trueBB);
    return nullptr;
}

Value *SysYBuilder::visit(ASTAddExp &node){
    if(node.add_exp == nullptr)
    {
        ////std::cout<< "mul_exp " << std::endl;
        node.mul_exp->accept(*this);
    }
    else
    {
       /* if(need_const)
        {
            node.add_exp->accept(*this);
            auto add_exp = context.cur_const;
            node.mul_exp->accept(*this);
            auto mul_exp = context.cur_const;
            if(node.op == OP_PLUS)
            {
               context.cur_const = add_exp + mul_exp;
               
            }
            else if(node.op == OP_MINUS)
            {
                context.cur_const = add_exp - mul_exp;
            }
            if(context.cur_val->get_type()->is_float_type())
            {
               context.cur_val = CONST_FP(context.cur_const);
            }
            else
            {
               context.cur_val = CONST_INT((int)(context.cur_const));
            }
            return nullptr;
        }*/
        ////std::cout<< "add_exp   + mul_exp" << std::endl;
        node.add_exp->accept(*this);
        auto add_exp = context.cur_val;
        node.mul_exp->accept(*this);
        auto mul_exp = context.cur_val;
        SysYType type = TYPE_INT;
        if(add_exp->get_type()->is_float_type() || mul_exp->get_type()->is_float_type())
        {
            type = TYPE_FLOAT;
            if(add_exp->get_type()->is_integer_type())
            {
                auto const_value = dynamic_cast<ConstantInt *>(add_exp);
                if(const_value)
                {
                    add_exp = CONST_FP((float)(const_value->get_value()));
                }
                else
                {
                    add_exp = builder->create_sitofp(add_exp,FLOAT_T);
                }
            }
            if(mul_exp->get_type()->is_integer_type())
            {
                auto const_value = dynamic_cast<ConstantInt *>(mul_exp);
                if(const_value)
                {
                    mul_exp = CONST_FP((float)(const_value->get_value()));
                }
                else
                {
                    mul_exp = builder->create_sitofp(mul_exp,FLOAT_T);
                }
            }
        }
        auto add_const_int = dynamic_cast<ConstantInt *>(add_exp);
        auto mul_const_int = dynamic_cast<ConstantInt *>(mul_exp);
        auto add_const_float = dynamic_cast<ConstantFP *>(add_exp);
        auto mul_const_float = dynamic_cast<ConstantFP *>(mul_exp);
        if(node.op == OP_PLUS)
        {
            if(type == TYPE_FLOAT)
            {
                if(add_const_float != nullptr && mul_const_float != nullptr)
                {
                    context.cur_val = CONST_FP(add_const_float->get_value() + mul_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fadd(add_exp,mul_exp);
                }
            }
            else
            {
                if(add_const_int != nullptr && mul_const_int != nullptr)
                {
                    ////std::cout<< add_const_int->get_value() << " " << mul_const_int->get_value() << std::endl;
                    context.cur_val = CONST_INT(add_const_int->get_value() + mul_const_int->get_value());
                }
                else
                {
                    context.cur_val = builder->create_iadd(add_exp,mul_exp);
                }
            }
        }
        else if(node.op == OP_MINUS)
        {
            if(type == TYPE_FLOAT)
            {
                if(add_const_float != nullptr && mul_const_float != nullptr)
                {
                    context.cur_val = CONST_FP(add_const_float->get_value() - mul_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fsub(add_exp,mul_exp);
                }
            }
            else
            {
                if(add_const_int != nullptr && mul_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(add_const_int->get_value() - mul_const_int->get_value());
                }
                else
                {
                context.cur_val = builder->create_isub(add_exp,mul_exp);
                }
            }
        }
    }       

    return nullptr;
}

Value *SysYBuilder::visit(ASTMulExp &node){
    if(node.mul_exp == nullptr)
    {
        ////std::cout<< "unary_exp " << std::endl;
        node.unary_exp->accept(*this);
    }
    else
    {
       // //std::cout<< "mul_exp   * unary_exp" << std::endl;
        node.mul_exp->accept(*this);
        auto mul_exp = context.cur_val;
     //   //std::cout<< "unary_exp1 " << std::endl;
        node.unary_exp->accept(*this);
        auto unary_exp = context.cur_val;
        SysYType type = TYPE_INT;
        if(mul_exp->get_type()->is_float_type() || unary_exp->get_type()->is_float_type())
        {
            type = TYPE_FLOAT;
            if(mul_exp->get_type()->is_integer_type())
            {
                auto const_value = dynamic_cast<ConstantInt *>(mul_exp);
                if(const_value != nullptr)
                {
                    mul_exp = CONST_FP((float)(const_value->get_value()));
                }
                else
                {
                    mul_exp = builder->create_sitofp(mul_exp,FLOAT_T);
                }
            }
            if(unary_exp->get_type()->is_integer_type())
            {
                auto const_value = dynamic_cast<ConstantInt *>(unary_exp);
                if(const_value != nullptr)
                {
                    unary_exp = CONST_FP((float)(const_value->get_value()));
                }
                else
                {
                    unary_exp = builder->create_sitofp(unary_exp,FLOAT_T);
                }
            }
        }
        auto mul_const_int = dynamic_cast<ConstantInt *>(mul_exp);
        auto unary_const_int = dynamic_cast<ConstantInt *>(unary_exp);
        auto mul_const_float = dynamic_cast<ConstantFP *>(mul_exp);
        auto unary_const_float = dynamic_cast<ConstantFP *>(unary_exp);
        switch(node.op)
        {
            case OP_MUL:
                if(type == TYPE_FLOAT)
                {
                    if(mul_const_float != nullptr && unary_const_float != nullptr)
                    {
                        context.cur_val = CONST_FP(mul_const_float->get_value() * unary_const_float->get_value());
                    }
                    else
                    {
                        //std::cout<< "mul_exp   * unary_exp fmul" << std::endl;
                        context.cur_val = builder->create_fmul(mul_exp,unary_exp);
                    }
                }
                else
                {
                    if(mul_const_int != nullptr && unary_const_int != nullptr)
                    {
                        context.cur_val = CONST_INT(mul_const_int->get_value() * unary_const_int->get_value());
                    }
                    else
                    {
                        context.cur_val = builder->create_imul(mul_exp,unary_exp);
                    }
                }
                break;
            case OP_DIV:
                if(type == TYPE_FLOAT)
                {
                    if(mul_const_float != nullptr && unary_const_float != nullptr)
                    {
                        context.cur_val = CONST_FP(mul_const_float->get_value() / unary_const_float->get_value());
                    }
                    else
                    {
                        context.cur_val = builder->create_fdiv(mul_exp,unary_exp);
                    }
                }
                else
                {
                    if(mul_const_int != nullptr && unary_const_int != nullptr)
                    {
                        context.cur_val = CONST_INT(mul_const_int->get_value() / unary_const_int->get_value());
                    }
                    else
                    {
                        context.cur_val = builder->create_isdiv(mul_exp,unary_exp);
                    }
                }
                break;
            case OP_MOD:
                {
                    if(mul_const_int != nullptr && unary_const_int != nullptr)
                    {
                        context.cur_val = CONST_INT(mul_const_int->get_value() % unary_const_int->get_value());
                    }
                    else
                    {
                        context.cur_val = builder->create_isrem(mul_exp,unary_exp);
                    }
                }
                break;
        }
    }
    return nullptr;
    
}

Value *SysYBuilder::visit(ASTUnaryExp &node){
    ////std::cout<< "unary" << std::endl;
    if(node.primary_exp)
    {
        node.primary_exp->accept(*this);   
    }
    else if(node.call)
    {
        node.call->accept(*this);
    }
    else
    {
        node.unary_exp->accept(*this);
        auto unary_exp = context.cur_val;
        auto unary_const_int = dynamic_cast<ConstantInt *>(unary_exp);
        auto unary_const_float = dynamic_cast<ConstantFP *>(unary_exp);
        if(node.op == OP_POS)
        {
            context.cur_val = unary_exp;
        }
        else if(node.op == OP_NEG)
        {
            if(unary_exp->get_type()->is_float_type())
            {
                if(unary_const_float != nullptr)
                {
                    context.cur_val = CONST_FP((-unary_const_float->get_value()));
                }
                else
                {
                    context.cur_val = builder->create_fsub(CONST_FP(0.),unary_exp);
                 //   context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(unary_const_int != nullptr)
                {
                    context.cur_val = CONST_INT((-unary_const_int->get_value()));
                }
                else
                {
                    ////std::cout<< "neg   " << std::endl; 
                    context.cur_val = builder->create_isub(CONST_INT(0),unary_exp);
              //  context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }
        else if(node.op == OP_NOT)
        {
            if(unary_exp->get_type()->is_float_type())
            {
                if(unary_const_float != nullptr)
                {
                    context.cur_val = CONST_FP(!unary_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fcmp_eq(unary_exp,CONST_FP(0.));
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(unary_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(!unary_const_int->get_value());
                }
                else
                {
                    context.cur_val = builder->create_icmp_eq(unary_exp,CONST_INT(0));
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }
        
    }
    return nullptr;
}

bool require_address = false;
Value *SysYBuilder::visit(ASTPrimaryExp &node){
    ////std::cout<< "primary" << std::endl;
    if(node.exp)
    {
        node.exp->accept(*this);
    }
    else if(node.l_val)
    {
       // //std::cout<< "lval  " << std::endl;
        node.l_val->accept(*this);
    }
    else if(node.number)
    {
        node.number->accept(*this);
    }

     return nullptr;   
}

Value *SysYBuilder::visit(ASTLVal &node){
    ////std::cout<< "lval   " << std::endl;
  // printf("%s\n",node.id.c_str());
    auto var = scope.find(node.id);
    ////std::cout<< need_lvalue << "   need_lvalue" << std::endl;
    bool require_lvalue = need_lvalue;
    need_lvalue = false;
    if(node.array_ident == nullptr)
    {
        if(require_lvalue)
        {
            //need_lvalue = false;
            if(var->get_type()->get_pointer_element_type()->is_array_type())
            {
                context.cur_val = builder->create_gep(var,{CONST_INT(0),CONST_INT(0)});
            }
            else if(var->get_type()->get_pointer_element_type()->is_pointer_type())
            {
                context.cur_val = builder->create_load(var);
            }
            else
            {
                context.cur_val = var;
            }
        }
        else
        {
            ////std::cout<< "lval get r val" << std::endl;
           // ////std::cout<< var->get_type() << std::endl;
            if(var->get_type() == FLOAT_T) {
                ////std::cout<< "float" << std::endl; 
                auto val_const = dynamic_cast<ConstantFP*>(var);
                if(val_const != nullptr) 
                    context.cur_val = val_const;
                else
                    context.cur_val = builder->create_load(var);      
            } 
            else if(var->get_type() == INT32_T) {
                auto val_const = dynamic_cast<ConstantInt*>(var);
                if(val_const != nullptr) 
                    context.cur_val = val_const;
                else
                {
                    context.cur_val = builder->create_load(var);
                }
            }
            else
            {
                ////std::cout<< var->get_type()->is_pointer_type() << std::endl;
                ////std::cout<< var->get_type()->get_pointer_element_type()->is_float_type() << std::endl;
                context.cur_val = builder->create_load(var);
            }
        }
        
    }
    else
    {
        ////std::cout<< "array lval" << std::endl;
        auto param_list = scope.find_params(node.id);
        auto const_array = scope.find_const(node.id);
        bool check = false;
        std::vector<Value *> array_params;
        int index = 0;
        if(const_array != nullptr)
        {
            ////std::cout<< "const array" << std::endl;
            check = true;
        }
        for(int i = 0;i < node.array_ident->exps.size();i ++)
        {
            node.array_ident->exps[i]->accept(*this);
            array_params.push_back(context.cur_val);
                if(check == true)
                {
                    auto const_value = dynamic_cast<ConstantInt *>(context.cur_val);
                    if(const_value == nullptr)
                    {
                        check = false;
                    }
                    else
                    {
                        ////std::cout<< param_list[i+1] <<"  param_list" <<std::endl;
                        index =  param_list[i+1] * const_value->get_value() + index;
                    }
                }
            }
            //std::cout<< "index" << index << std::endl;
            Value *var_index  = nullptr;
            if(check == true && require_lvalue == false)
            {
                auto const_value = dynamic_cast<ConstantInt *>(const_array->get_element_value(index));
                context.cur_val = CONST_INT(const_value->get_value());
            }
            else
            {
                for(int i = 1;i <= array_params.size();i++)
                {
                    auto index = array_params[i-1];
                    Value *index_val;
                    if(param_list[i] != 1)
                    {
                        index_val = builder->create_imul(index,CONST_INT(param_list[i]));
                    }
                    else
                    {
                        index_val = index;
                    }
                    if(var_index == nullptr)
                    {
                        var_index = index_val;
                    }
                    else
                    {
                        var_index = builder->create_iadd(var_index,index_val);
                    }
                }
                if(var->get_type()->get_pointer_element_type()->is_pointer_type())
                {
                    auto load = builder->create_load(var);
                    context.cur_val = builder->create_gep(load,{var_index});
                }
                else
                {
                    context.cur_val = builder->create_gep(var,{CONST_INT(0),var_index});
                }
                if(!require_lvalue)
                {
                    context.cur_val = builder->create_load(context.cur_val);
                }
            }
            
        
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTNumber &node){
    if(node.type == TYPE_INT)
    {
        ////std::cout<< "number " << node.int_val<<std::endl;
        context.cur_val = CONST_INT(node.int_val);
    }
    else
    {
      //  //std::cout<< "number float " << node.float_val<<std::endl;
        context.cur_val = CONST_FP(node.float_val);
    }
    return nullptr;

}

Value *SysYBuilder::visit(ASTLOrExp &node){
    if(node.l_or_exp == nullptr)
    {
        ////std::cout<< "lor_exp  land_exp" << std::endl;
        node.l_and_exp->accept(*this);
         if(context.cur_val->get_type()->is_int1_type())
            context.cur_val = builder->create_zext(context.cur_val,INT32_T);
        //context.cur_val = builder->create_icmp_ne(context.cur_val,CONST_INT(0));
    }
    else
    {
        ////std::cout<< "lor_exp" << std::endl;
        auto falseBB = BasicBlock::create(module.get(),"falseBB"+std::to_string(if_count),context.func);
        if_count ++;
        false_stack.push_back(falseBB);
        node.l_or_exp->accept(*this);
        false_stack.pop_back();
        auto l_or_exp = context.cur_val;
        if(l_or_exp->get_type()->is_float_type())
        {
         //   auto const_value = dynamic_cast<ConstantFP *>(l_or_exp);
         //   if(const_value)
          //  {
          //      l_or_exp = CONST_INT(const_value->get_value() != 0);
         //   }
          //  else
         //   {
                l_or_exp = builder->create_fcmp_ne(l_or_exp,CONST_FP(0.));
          //  }
        }
        else
        {
          //  auto const_value = dynamic_cast<ConstantInt *>(l_or_exp);
          //  if(const_value)
           // {
           //     l_or_exp = CONST_INT(const_value->get_value() != 0);
           // }
          //  else
          //  {
                l_or_exp = builder->create_icmp_ne(l_or_exp,CONST_INT(0));
          //  }
        }
        context.cur_val = builder->create_zext(l_or_exp,INT32_T);
        builder->create_cond_br(l_or_exp,true_stack.back(),falseBB);
        builder->set_insert_point(falseBB);
        node.l_and_exp->accept(*this);
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTLAndExp &node){
    if(node.l_and_exp == nullptr)
    {
        ////std::cout<< "landexp  eqexp" << std::endl;
        node.eq_exp->accept(*this);
        if(context.cur_val->get_type()->is_int1_type())
            context.cur_val = builder->create_zext(context.cur_val,INT32_T);
        ////std::cout<< "landexp  eqexp finish" << std::endl;
    }
    else
    {
        ////std::cout<< "landexp" << std::endl;
        auto trueBB = BasicBlock::create(module.get(),"trueBB"+std::to_string(if_count),context.func);
        if_count ++;
        node.l_and_exp->accept(*this);
        auto l_and_exp = context.cur_val;
        ////std::cout<< l_and_exp->get_type()->is_integer_type() << " type check " << std::endl;
        if(l_and_exp->get_type()->is_float_type())
        {
           // auto const_value = dynamic_cast<ConstantFP *>(l_and_exp);
            //if(const_value)
           /// {
           //     l_and_exp = CONST_INT(const_value->get_value() != 0);
            //}
           // else
           // {
                l_and_exp = builder->create_fcmp_ne(l_and_exp,CONST_FP(0.));
           // }
        }
        else
        {
           // auto const_value = dynamic_cast<ConstantInt *>(l_and_exp);
           // if(const_value)
          //  {
          //      l_and_exp = CONST_INT(const_value->get_value() != 0);
          //  }
          //  else
           // {
                l_and_exp = builder->create_icmp_ne(l_and_exp,CONST_INT(0));
          //  }
        }
        context.cur_val = builder->create_zext(l_and_exp,INT32_T);
        builder->create_cond_br(l_and_exp,trueBB,false_stack.back());
        ////std::cout<< "landexp  eqexp finish" << std::endl;
        builder->set_insert_point(trueBB);
        node.eq_exp->accept(*this);

    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTRelExp &node){
    if(node.rel_exp == nullptr)
    {
        ////std::cout<< "rel_exp   add_exp" << std::endl;
        node.add_exp->accept(*this);
    }
    else
    {
        ////std::cout<< "rel_exp   rel_exp" << std::endl; 
        node.rel_exp->accept(*this);
        auto rel_exp = context.cur_val;
        ////std::cout<< "rel_exp   add_exp2222" << std::endl;
        node.add_exp->accept(*this);
        auto add_exp = context.cur_val;
        SysYType type = TYPE_INT;
        if(rel_exp->get_type()->is_float_type() || add_exp->get_type()->is_float_type())
        {
            type = TYPE_FLOAT;
            if(rel_exp->get_type()->is_integer_type())
            {
                rel_exp = builder->create_sitofp(rel_exp,FLOAT_T);
            }
            if(add_exp->get_type()->is_integer_type())
            {
                add_exp = builder->create_sitofp(add_exp,FLOAT_T);
            }
        }
        ////std::cout<< (type == TYPE_INT) << " type  "<< std::endl;
        auto rel_const_int = dynamic_cast<ConstantInt *>(rel_exp);
        auto add_const_int = dynamic_cast<ConstantInt *>(add_exp);
        auto rel_const_float = dynamic_cast<ConstantFP *>(rel_exp);
        auto add_const_float = dynamic_cast<ConstantFP *>(add_exp);
        if(node.op == OP_LE)
        {
            if(type == TYPE_FLOAT)
            {
                if(rel_const_float != nullptr && add_const_float != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_float->get_value() <= add_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fcmp_le(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(rel_const_int != nullptr && add_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_int->get_value() <= add_const_int->get_value());
                }
                else
                {
                    context.cur_val = builder->create_icmp_le(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }
        else if(node.op == OP_LT)
        {
            if(type == TYPE_FLOAT)
            {
                if(rel_const_float != nullptr && add_const_float != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_float->get_value() < add_const_float->get_value());
                }
                else
                {
                   
                    ////std::cout<< "rel_exp   add_exp   lt" << std::endl;
                    context.cur_val = builder->create_fcmp_lt(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(rel_const_int != nullptr && add_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_int->get_value() < add_const_int->get_value());
                }
                else
                {
                    ////std::cout<< rel_exp->get_type()->is_int32_type() << " type check " << add_exp->get_type()->is_int32_type() << std::endl;
                    ////std::cout<< "rel_exp   add_exp   lt" << std::endl;
                    context.cur_val = builder->create_icmp_lt(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }
        else if(node.op == OP_GE)
        {
            if(type == TYPE_FLOAT)
            {
                if(rel_const_float != nullptr && add_const_float != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_float->get_value() >= add_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fcmp_ge(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(rel_const_int != nullptr && add_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_int->get_value() >= add_const_int->get_value());
                }
                else
                {
                    context.cur_val = builder->create_icmp_ge(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }   
        else if(node.op == OP_GT)
        {
            if(type == TYPE_FLOAT)
            {
                if(rel_const_float != nullptr && add_const_float != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_float->get_value() > add_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fcmp_gt(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(rel_const_int != nullptr && add_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(rel_const_int->get_value() > add_const_int->get_value());
                }
                else
                {
                    context.cur_val = builder->create_icmp_gt(rel_exp,add_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }
       
        
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTEqExp &node){
    if(node.eq_exp == nullptr)
    {
        ////std::cout<< "eq_exp   rel_exp" << std::endl;
        node.rel_exp->accept(*this);
    }
    else
    {
        node.eq_exp->accept(*this);
        auto eq_exp = context.cur_val;
        node.rel_exp->accept(*this);
        auto rel_exp = context.cur_val;
        SysYType type = TYPE_INT;
        if(eq_exp->get_type()->is_float_type() || rel_exp->get_type()->is_float_type())
        {
            type = TYPE_FLOAT;
            if(eq_exp->get_type()->is_integer_type())
            {
                eq_exp = builder->create_sitofp(eq_exp,FLOAT_T);
            }
            if(rel_exp->get_type()->is_integer_type())
            {
                rel_exp = builder->create_sitofp(rel_exp,FLOAT_T);
            }
        }
        auto eq_const_int = dynamic_cast<ConstantInt *>(eq_exp);
        auto rel_const_int = dynamic_cast<ConstantInt *>(rel_exp);
        auto eq_const_float = dynamic_cast<ConstantFP *>(eq_exp);
        auto rel_const_float = dynamic_cast<ConstantFP *>(rel_exp);
        if(node.op == OP_EQ)
        {
            if(type == TYPE_FLOAT)
            {
                if(eq_const_float != nullptr && rel_const_float != nullptr)
                {
                    context.cur_val = CONST_INT(eq_const_float->get_value() == rel_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fcmp_eq(eq_exp,rel_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(eq_const_int != nullptr && rel_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(eq_const_int->get_value() == rel_const_int->get_value());
                }
                else
                {
                    context.cur_val = builder->create_icmp_eq(eq_exp,rel_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }
        else if(node.op == OP_NEQ)
        {
            if(type == TYPE_FLOAT)
            {
                if(eq_const_float != nullptr && rel_const_float != nullptr)
                {
                    context.cur_val = CONST_INT(eq_const_float->get_value() != rel_const_float->get_value());
                }
                else
                {
                    context.cur_val = builder->create_fcmp_ne(eq_exp,rel_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
            else
            {
                if(eq_const_int != nullptr && rel_const_int != nullptr)
                {
                    context.cur_val = CONST_INT(eq_const_int->get_value() != rel_const_int->get_value());
                }
                else
                {
                    context.cur_val = builder->create_icmp_ne(eq_exp,rel_exp);
                    context.cur_val = builder->create_zext(context.cur_val,INT32_T);
                }
            }
        }
 
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTCall &node){
    ////std::cout<< node.id << "  call" << std::endl;
    //LOG_DEBUG << "call " << node.id;
    Function *fun = (Function *)scope.find_func(node.id);
    std::vector<Value *> args;
    int i = 0;

    if(node.id == "starttime" || node.id == "stoptime")
    {
        args.push_back(CONST_INT(node.lineno));
    }
    else
    {
        for(auto &arg : node.exps)
        {
            auto arg_type = fun->get_function_type()->get_param_type(i);
            i ++;
            if(arg_type->is_integer_type() || arg_type->is_float_type())
            {
                need_lvalue = false;
            }
            else
            {
                need_lvalue = true;
            }
            arg->accept(*this);
            need_lvalue = false;
            if(arg_type->is_float_type() && context.cur_val->get_type()->is_integer_type())
            {
                auto const_value = dynamic_cast<ConstantInt *>(context.cur_val);
                if(const_value)
                {
                    context.cur_val = CONST_FP(const_value->get_value());
                }
                else
                {
                    context.cur_val = builder->create_sitofp(context.cur_val,FLOAT_T);
                }
            }
            else if(arg_type->is_integer_type() && context.cur_val->get_type()->is_float_type())
            {
                auto const_value = dynamic_cast<ConstantFP *>(context.cur_val);
                if(const_value)
                {
                    context.cur_val = CONST_INT((int)(const_value->get_value()));
                }
                else
                {
                    context.cur_val = builder->create_fptosi(context.cur_val,INT32_T);
                }
            }
            args.push_back(context.cur_val);
            arg_type++;
        }
    }
    context.cur_val = builder->create_call(fun,args);
    ////std::cout<< "call end" << std::endl;
    return nullptr;
}

Value *SysYBuilder::visit(ASTConstExp &node){
   // need_const = true;
    node.add_exp->accept(*this);
    //context.cur_val = 
    //need_const = false;
    return nullptr;
}

Value *SysYBuilder::visit(ASTDecl &node){return nullptr;}

Value *SysYBuilder::visit(ASTFuncFParam &node){return nullptr;}

Value *SysYBuilder::visit(ASTBlockItem &node){return nullptr;}

Value *SysYBuilder::visit(ASTStmt &node){return nullptr;}

Value *SysYBuilder::visit(ASTExp &node){return nullptr;}

Value *SysYBuilder::visit(ASTCond &node){return nullptr;}