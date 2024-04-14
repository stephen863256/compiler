#include "InstCombine.hpp"
#include "Constant.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include "Dominators.hpp"

std::set<InstCombine::Expr*> visited;
void InstCombine::run() {
    Dominators dom(module_);
  //  dom.run();    
    for (auto &func : module_->get_functions()) {
        auto func_ = &func;
        if(func_->is_declaration()) continue;
        dom.create_reverse_post_order(func_);
        for (auto &bb:dom.get_reverse_post_order())
        {
            for(auto &inst:bb->get_instructions())
            {
                if(inst.is_add() || inst.is_sub() || inst.is_mul() || inst.is_div())
                {
                   // LOG_DEBUG << "build expr tree for " << inst.print();
                    build_expr_tree(&inst);
                }
            }
        }

        visited.clear();
        for(auto &expr: root_expr_list)
        {
          //  LOG_DEBUG << "reassociation for " << expr->val->print() << "   " << func_->get_name();
            reassociation(expr);
          //  LOG_DEBUG << "expr257 " << expr->const_val;
        }
        
        combine_instruction(func_);
        expr_map.clear();
        root_expr_list.clear();
      //  LOG_DEBUG << module_->print();
    }
    for(auto &func: module_->get_functions())
    {
        for(auto &bb:func.get_basic_blocks())
        {
            for(auto &inst:bb.get_instructions())
            {
                if(inst.is_add())
                {
                    auto lhs = inst.get_operand(0);
                    auto rhs = inst.get_operand(1);
                    if(dynamic_cast<ConstantInt *>(lhs))
                    {
                        auto constant = dynamic_cast<ConstantInt *>(lhs);
                        if(constant->get_value() == 0)
                        {
                            inst.replace_all_use_with(rhs);
                        }
                    }
                    else if(dynamic_cast<ConstantInt *>(rhs))
                    {
                        auto constant = dynamic_cast<ConstantInt *>(rhs);
                        if(constant->get_value() == 0)
                        {
                            inst.replace_all_use_with(lhs);
                        }
                    }
                }    
            }
        }
    }
}

void InstCombine::build_expr_tree(Instruction *inst)
{
    if(expr_map.find(inst) == expr_map.end())
    {
        auto lhs = get_expr(inst->get_operand(0));
        auto rhs = get_expr(inst->get_operand(1));
        Expr *expr = new Expr(lhs,rhs,inst,module_);
        expr_map[inst] = expr;
        root_expr_list.push_back(expr);
        root_expr_list.remove(lhs);
        root_expr_list.remove(rhs);
    }
    else 
    {
        auto expr = expr_map[inst];
        auto lhs = get_expr(inst->get_operand(0));
        auto rhs = get_expr(inst->get_operand(1));
        expr->lhs = lhs;
        expr->rhs = rhs;    
    }
}

InstCombine::Expr *InstCombine::get_expr(Value *val)
{
    if(expr_map.find(val) != expr_map.end())
    {
        return expr_map[val];
    }
    else
    {
        Expr *expr = new Expr(nullptr,nullptr,val,module_);
        expr_map[val] = expr;
        return expr;
    }
}


void InstCombine::reassociation(Expr *expr)
{
    if(visited.find(expr) != visited.end()) return;
   // LOG_DEBUG << "reassociation for1234 " << expr->val->print();
    if(expr->op == Expr::TERM)
    {
        if(!expr->is_const)
        {
            expr->add_term(expr->val,true,1);
        }
        visited.insert(expr);
        return;
    }

    reassociation(expr->lhs);
    reassociation(expr->rhs);

    if(expr->op == Expr::ADD || expr->op == Expr::SUB)
    {
        combine_add_sub_expr(expr);
    }
    else if(expr->op == Expr::MUL)
    {
        combine_mul_expr(expr);
    }
    else if(expr->op == Expr::DIV)
    {
        combine_div_expr(expr);
    }
    visited.insert(expr);
}
void InstCombine::combine_add_sub_expr(Expr *expr)
{
    expr->need_replace = true;
    expr->insert_expr_term(expr->lhs,1);
    if(expr->op == Expr::SUB)
        expr->insert_expr_term(expr->rhs,-1);
    else
        expr->insert_expr_term(expr->rhs,1);
}

void InstCombine::combine_mul_expr(Expr *expr)
{
    if(expr->lhs->is_const && expr->rhs->is_const)
    {
        expr->const_val = expr->lhs->const_val * expr->rhs->const_val;
        expr->is_const = true;
    }
    else if(expr->lhs->is_const)
    {
        expr->insert_expr_term(expr->rhs,expr->lhs->const_val);
        no_user_expr_list.insert(expr->rhs);
        expr->need_replace = true;
        expr->is_const = false;
    }
    else if(expr->rhs->is_const)
    {
        expr->insert_expr_term(expr->lhs,expr->rhs->const_val);
        no_user_expr_list.insert(expr->lhs);
        expr->need_replace = true;
        expr->is_const = false;
    }
    else
    {
        expr->add_term(expr->val,true,1);
    }
}

void InstCombine::combine_div_expr(Expr *expr)
{
    if(expr->lhs->is_const && expr->rhs->is_const)
    {
        expr->const_val = expr->lhs->const_val / expr->rhs->const_val;
        expr->is_const = true;
    }
    else if(expr->rhs->is_const)
    {
        bool can_be_div = true;
        for(auto &term: expr->lhs->term_map)
        {
            if(term.second % expr->rhs->const_val != 0)
            {
                can_be_div = false;
                break;
            }
        }
        if(can_be_div)
        {
            expr->insert_expr_term(expr->lhs,1);
            no_user_expr_list.insert(expr->lhs);
            expr->is_const = false;
            expr->need_replace = true;
            for(auto &term: expr->term_map)
            {
                if(term.first->exp == true)
                {
                    expr->term_map[term.first] = term.second / expr->rhs->const_val;
                }
                else
                {
                    expr->term_map[term.first] = term.second * expr->rhs->const_val;
                }
            }
            expr->const_val /= expr->rhs->const_val;
            return;
        }
    }
    
    expr->add_term(expr->val,true,1);
}

void InstCombine::combine_instruction(Function *func)
{
    for(auto &bb: func->get_basic_blocks())
    {
        auto term = bb.get_terminator();
        bb.get_instructions().remove(term);
        for(auto &inst: bb.get_instructions())
        {
            if(inst.is_add() || inst.is_sub() || inst.is_mul() || inst.is_div())
            {
               // auto expr = expr_map[&inst];
               // if(no_user_expr_list.find(expr) != no_user_expr_list.end())
                //{
                 //   continue;
                //}
                if(expr_map.find(&inst) != expr_map.end())
                {
                    auto expr = expr_map[&inst];
                    if(expr->need_replace)
                    {
                        if(expr->is_const)
                        {
                            auto constant = ConstantInt::get(expr->const_val,module_);
                            inst.replace_all_use_with(constant);
                        }
                        else
                        {
                            std::map<int,std::vector<Value*>> mul_map;
                            std::map<int,std::vector<Value*>> div_map;
                            for(auto term:expr->term_map)
                            {
                                if(term.first->exp == true)
                                {
                                    if(mul_map.find(term.second) == mul_map.end())
                                    {
                                        mul_map.insert({term.second,{term.first->val}});
                                    }
                                    else
                                    {
                                        mul_map[term.second].push_back(term.first->val);
                                    }
                                }
                                else
                                {
                                    if(div_map.find(term.second) == div_map.end())
                                    {
                                        div_map.insert({term.second,{term.first->val}});
                                    }
                                    else
                                    {
                                        div_map[term.second].push_back(term.first->val);
                                    }
                                }
                            }
                            auto cur_inst = &inst;
                            Value *new_inst = ConstantInt::get(0,module_);
                            for(auto &mul:mul_map)
                            {
                                Value *mul_val = nullptr;
                                if(mul.first == 0)
                                {
                                    continue;
                                }
                                for(auto &val:mul.second)
                                {
                                    if(mul_val == nullptr)
                                    {
                                        mul_val = val;
                                    }
                                    else
                                    {
                                        mul_val = IBinaryInst::create_add(mul_val,val,&bb);
                                        bb.insert_instruction(cur_inst,static_cast<Instruction *>(mul_val));
                                        cur_inst = static_cast<Instruction *>(mul_val);
                                    }
                                }
                                if(mul.first > 0)
                                {
                                    if(mul.first != 1)
                                    {
                                        mul_val = IBinaryInst::create_mul(mul_val,ConstantInt::get(mul.first,module_),&bb);
                                        bb.insert_instruction(cur_inst,static_cast<Instruction *>(mul_val));
                                        cur_inst = static_cast<Instruction *>(mul_val);
                                    }
                                    new_inst = IBinaryInst::create_add(new_inst,mul_val,&bb);
                                    bb.insert_instruction(cur_inst,static_cast<Instruction *>(new_inst));
                                    cur_inst = static_cast<Instruction *>(new_inst);
                                }
                                else
                                {
                                    if(mul.first != -1)
                                    {
                                        mul_val = IBinaryInst::create_mul(mul_val,ConstantInt::get(-mul.first,module_),&bb);
                                        bb.insert_instruction(cur_inst,static_cast<Instruction *>(mul_val));
                                        cur_inst = static_cast<Instruction *>(mul_val);
                                    }
                                    new_inst = IBinaryInst::create_sub(new_inst,mul_val,&bb);
                                    bb.insert_instruction(cur_inst,static_cast<Instruction *>(new_inst));
                                    cur_inst = static_cast<Instruction *>(new_inst);
                                }
                               // new_inst = IBinaryInst::create_add(new_inst,mul_val,&bb);
                                //bb.insert_instruction(cur_inst,static_cast<Instruction *>(new_inst));
                                //cur_inst = static_cast<Instruction *>(new_inst);
                            }
                            for(auto &div:div_map)
                            {
                                Value *div_val = nullptr;
                                if(div.first == 0)
                                {
                                    continue;
                                }
                                for(auto &val:div.second)
                                {
                                    if(div_val == nullptr)
                                    {
                                        div_val = val;
                                    }
                                    else
                                    {
                                        div_val = IBinaryInst::create_add(div_val,val,&bb);
                                        bb.insert_instruction(cur_inst,static_cast<Instruction *>(div_val));
                                        cur_inst = static_cast<Instruction *>(div_val);
                                    }
                                }
                                if(div.first > 0)
                                {
                                    if(div.first != 1)
                                    {
                                        div_val = IBinaryInst::create_sdiv(div_val,ConstantInt::get(div.first,module_),&bb);
                                        bb.insert_instruction(cur_inst,static_cast<Instruction *>(div_val));
                                        cur_inst = static_cast<Instruction *>(div_val);
                                    }
                                    new_inst = IBinaryInst::create_add(new_inst,div_val,&bb);
                                    bb.insert_instruction(cur_inst,static_cast<Instruction *>(new_inst));
                                    cur_inst = static_cast<Instruction *>(new_inst);
                                }
                                else
                                {
                                    if(div.first != -1)
                                    {
                                        div_val = IBinaryInst::create_sdiv(div_val,ConstantInt::get(-div.first,module_),&bb);
                                        bb.insert_instruction(cur_inst,static_cast<Instruction *>(div_val));
                                        cur_inst = static_cast<Instruction *>(div_val);
                                    }
                                    new_inst = IBinaryInst::create_sub(new_inst,div_val,&bb);
                                    bb.insert_instruction(cur_inst,static_cast<Instruction *>(new_inst));
                                    cur_inst = static_cast<Instruction *>(new_inst);
                                }
                                //new_inst = IBinaryInst::create_add(new_inst,div_val,&bb);
                                //bb.insert_instruction(cur_inst,static_cast<Instruction *>(new_inst));
                                //cur_inst = static_cast<Instruction *>(new_inst);
                            }
                            if(expr->const_val != 0)
                            {
                                new_inst = IBinaryInst::create_add(new_inst,ConstantInt::get(expr->const_val,module_),&bb);
                            }
                            bb.insert_instruction(cur_inst,static_cast<Instruction *>(new_inst));
                            inst.replace_all_use_with(new_inst);
                        }
                    }
                }
            }
        }
        bb.add_instruction(term);
    }
}
