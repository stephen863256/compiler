#pragma once

#include "Function.hpp"
#include "Instruction.hpp"
#include "PassManager.hpp"
#include "Module.hpp"
#include "Value.hpp"
#include "Dominators.hpp"
#include "logging.hpp"
#include <unordered_map>



class InstCombine : public Pass {
    private:
        Module *module_;
        //Dominators *dominators_;
    public:
        InstCombine(Module *m) : Pass(m) {module_ = m;}
        void run() override;

        struct Term{
            Value *val;
            bool exp;// 1 代表乘，0 代表除
            Term(Value *val, bool exp) : val(val), exp(exp) {}
        };
        struct Expr{
        public:
            Expr *lhs;
            Expr *rhs;
            Value *val;
            Module *m;
            bool is_const = false;
            int const_val = 0;
            bool need_replace = false;
            enum ExprOP{ADD, SUB, MUL, DIV, TERM} op;

            Expr(Expr *lhs, Expr *rhs, Value *val, Module *m) : lhs(lhs), rhs(rhs), val(val), m(m) {
                if(lhs == nullptr && rhs == nullptr) 
                {
                    op = TERM;
                    auto constant = dynamic_cast<ConstantInt *>(val);
                    if(constant != nullptr) 
                    {
                        is_const = true;
                        const_val = constant->get_value();
                    }
                }
                else 
                {
                    auto inst = static_cast<Instruction *>(val);
                    if(inst->is_add()) op = ADD;
                    else if(inst->is_sub()) op = SUB;
                    else if(inst->is_mul()) op = MUL;
                    else if(inst->is_div()) op = DIV;
                }
            }
            int get_value() {return const_val;}
            std::unordered_map<Term*, int > term_map;

            void add_term(Value *val, bool exp,int n){
                auto constant = dynamic_cast<ConstantInt *>(val);
                if(constant != nullptr) 
                {
                    if(constant->get_value()!=0)
                        need_replace = true;
                    const_val += constant->get_value()*n;
                    return;
                }
              //  auto term = new Term(val, exp);
                Term *ptr = nullptr;
                for(auto &term: term_map)
                {
                    if(term.first->val == val && term.first->exp == exp)
                    {
                        ptr = term.first;
                        term_map[term.first] += n;
                        break;
                    }
                }
                if(ptr == nullptr)
                {
                    auto term = new Term(val, exp);
                    term_map[term] = n;
                }
            }
            void insert_expr_term(Expr *expr,int n)
            {
                if(expr->op == TERM)
                {
                    //LOG_DEBUG << "insert_expr_term";
                    add_term(expr->val,true,n);
                }
                else
                {
                    for(auto &term: expr->term_map)
                    {
                        if(term.first->exp == false && (n % term.second !=0 && term.second % n != 0))
                        {
                            add_term(term.first->val,true,n);
                            return;
                        }
                    }

                    for(auto &term: expr->term_map)
                    {
                        int t = term.second ;
                        if(term.first->exp == true)
                        {
                            add_term(term.first->val,true,n*t);
                        }
                        else if(n % t == 0)
                        {
                            if(n == t)
                            {
                                add_term(term.first->val,true,1);
                            }
                            else
                            {
                                add_term(term.first->val,true,n/t);
                                //  *(n/t)   
                            }
                        }
                        else if(t % n == 0)
                        {
                            add_term(term.first->val,false,t/n);
                            //  /(t/n)
                        }
                    }
                    if(expr->const_val != 0)
                    {
                        const_val += expr->const_val * n;
                        need_replace = true;
                    }
                }
            }
        };

        std::map<Value *, Expr *> expr_map;
        std::list<Expr *> root_expr_list; 
        std::set<Expr *> no_user_expr_list;

        void build_expr_tree(Instruction *inst);
        Expr *get_expr(Value *val);
        void reassociation(Expr *expr);
        void combine_add_sub_expr(Expr *expr);
        void combine_mul_expr(Expr *expr);
        void combine_div_expr(Expr *expr);
        void combine_instruction(Function *func);
};