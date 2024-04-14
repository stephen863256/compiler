#pragma once

#include <queue>

#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Module.hpp"
#include "Instruction.hpp"
#include "Value.hpp"

struct Range {
    int from;
    int to;
    Range(int from, int to): from(from), to(to) {}
};

class Interval {
public:
    explicit Interval(Value *value): value(value), is_fp(value->get_type()->is_float_type()) {}

public:
    void add_range(int from, int to);
    void add_use_pos(int pos){ use_position_list.push_back(pos);}
    bool covers(int id)
    {
        for(auto &range: range_list)
        {
            if(range->from <= id && range->to > id)
            {
                return true;
            }
        }
        return false;
    }
    bool covers(Instruction *inst){ return covers(inst->get_id());}
    bool intersects(Interval *other);
    void union_interval(Interval *other);
    bool is_float_type() { return is_fp; }

public:
    int reg_id = -1;
    std::list<Range*> range_list;
    std::list<int> use_position_list;
    Value *value;
    bool is_fp;
};

/*const int ireg_priority[] = {
    0, 0,
    0, 0,
    0, 25,
    24, 23,
    0, 0,
    8, 7,
    6, 5,
    4, 3,
    2, 1,
    18, 17,
    16, 15,
    14, 13,
    12, 11,
    10, 9,
    22, 21,
    20, 19
};*/

const int ireg_priority[] = {
    0, 0,  // 0-1 不能分配
    0, 0,  // 2-3 不能分配
    8, 7,  // 4-5 传参和返回值寄存器
    6, 5, 4, 3, 2, 1,  // 6-11 传参寄存器
    15, 14, 13, 12, 11, 10, 9, 0,0,// 12-20 临时寄存器
    0,  // 21 不能分配
    0,  // 22 栈帧指针
    0, 0, 0, 0, 0, 0, 0, 0, 0  // 23-31 静态寄存器
};


/*const int freg_priority[] = {
    30, 29,
    28, 27,
    26, 25,
    24, 23,
    0, 0,
    8, 7,
    6, 5,
    4, 3,
    2, 1,
    18, 17,
    16, 15,
    14, 13,
    12, 11,
    10, 9,
    22, 21,
    20, 19
};*/

const int freg_priority[] = {
    8, 7,  // 0-1 传参和返回值寄存器
    6, 5, 4, 3, 2, 1,  // 2-7 传参寄存器
    22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 0, 0,// 8-23 临时寄存器
    0, 0, 0, 0, 0, 0, 0, 0  // 24-31 优先级设置为0
};

/*const std::vector<int> all_available_ireg_ids = {
    5, 6, 7, 
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31 
};*/

const std::vector<int> all_available_ireg_ids = {
    4, 5,  // 传参和返回值寄存器
    6, 7, 8, 9, 10, 11,  // 传参寄存器
    12, 13, 14, 15, 16, 17, 18 // 临时寄存器
};

const std::vector<int> all_available_freg_ids = {
    0, 1,  // 传参和返回值寄存器
    2, 3, 4, 5, 6, 7,  // 传参寄存器
    8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21// 临时寄存器
};
/*const std::vector<int> all_available_freg_ids = {
    0, 1, 2, 3, 4, 5, 6, 7,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31
};*/

class RegAllocDriver {
public:
    explicit RegAllocDriver(Module *m): m(m) {}
public:
    void compute_reg_alloc();
    std::map<Value*, Interval*>& get_ireg_alloc_in_func(Function *func) { return ireg_alloc[func]; }
    std::map<Value*, Interval*>& get_freg_alloc_in_func(Function *func) { return freg_alloc[func]; }
    std::list<BasicBlock*>& get_block_order_in_func(Function *func) { return block_order[func]; }
private:
    std::map<Function*, std::map<Value*, Interval*>> ireg_alloc;
    std::map<Function*, std::map<Value*, Interval*>> freg_alloc;
    std::map<Function*, std::list<BasicBlock*>> block_order;
    Module *m;
};


struct cmp_interval {
    bool operator()(const Interval *a, const Interval *b) const {
        auto a_from = (*(a->range_list.begin()))->from;
        auto b_from = (*(b->range_list.begin()))->from;
        if(a_from!=b_from){
            return a_from < b_from;
        }else{
            return a->value->get_name() < b->value->get_name();
        }
    }
};


struct cmp_ireg {
    bool operator()(const int reg1,const int reg2) const{
        return ireg_priority[reg1] > ireg_priority[reg2];
    }
};

struct cmp_freg {
    bool operator()(const int reg1,const int reg2)const{
        return freg_priority[reg1] > freg_priority[reg2];
    }
};

struct cmp_range{
    bool operator()(const Range* a,const Range* b) const {
        return a->from > b->from;
    }
};

struct cmp_block_depth{
    bool operator()(BasicBlock* b1, BasicBlock* b2){
        return b1->get_loop_depth() < b2->get_loop_depth();
    }
};

class RegAlloc {
public:
    explicit RegAlloc(Function* func): func(func) {}

public:
    void run();
    std::map<Value*, Interval*>& get_ireg_alloc() { return ival2Inter; }
    std::map<Value*, Interval*>& get_freg_alloc() { return fval2Inter; }
    std::list<BasicBlock*>& get_block_order() { return block_order; }

private:
    void init();
    void compute_block_order();
    void number_operations();
    void compute_bonus_and_cost();
    void build_intervals();
    void walk_intervals();
    void add_interval(Interval *interval) { interval_list.insert(interval); }
    void add_reg_to_pool(Interval *interval);
    void alloc_reg();

private:
    std::set<int> unused_ireg_id = { all_available_ireg_ids.begin(), all_available_ireg_ids.end() };
    std::set<int, cmp_ireg> remained_all_ireg_ids = { all_available_ireg_ids.begin(), all_available_ireg_ids.end() };
    std::map<int,std::set<Interval*>> ireg2ActInter;

    std::set<int> unused_freg_id = { all_available_freg_ids.begin(), all_available_freg_ids.end() };
    std::set<int, cmp_freg> remained_all_freg_ids = { all_available_freg_ids.begin(), all_available_freg_ids.end() };
    std::map<int,std::set<Interval*>> freg2ActInter;

    std::set<Interval*> active = {};
    Interval *current = nullptr;
    std::map<Value*, Interval*> ival2Inter;
    std::map<Value*, Interval*> fval2Inter;
    Function* func;
    std::list<BasicBlock*> block_order={};
    std::set<Interval*,cmp_interval> interval_list;

    //& spill cost
    const double load_cost = 5.0;
    const double store_cost = 3.0;
    const double loop_scale = 100.0;
    const double mov_cost = 1.0;

    std::map<Value* ,double> spill_cost;

    //& bonus
    std::map<Value* ,std::map<Value*, double>> phi_bonus;
    std::map<Value* ,std::map<int, double>> caller_arg_bonus;
    std::map<Value* ,std::map<int, double>> callee_arg_bonus;
    std::map<Value* ,double> call_bonus;
    std::map<Value* ,double> ret_bonus;
};

class ActiveVar {
    public:
        void run();
        void get_def_use(Function *func);
        void get_in_out(Function *func);
        std::set<Value *> get_live_in_int(BasicBlock *bb) { return ilive_in1[bb]; }
        std::set<Value *> get_live_out_int(BasicBlock *bb) { return ilive_out1[bb]; }
        std::set<Value *> get_live_in_float(BasicBlock *bb) { return flive_in1[bb]; }
        std::set<Value *> get_live_out_float(BasicBlock *bb) { return flive_out1[bb]; }
        ActiveVar(Module *m): m(m) {}
       
    private:
        Module *m;    
        std::map<BasicBlock *, std::set<Value *>> use_map,def_map,phi_map;
        std::map<BasicBlock *, std::set<Value *>> flive_in, flive_out;
        std::map<BasicBlock *, std::set<Value *>> ilive_in, ilive_out;
        std::map<BasicBlock *, std::set<Value *>> use_map1,def_map1,phi_map1;
        std::map<BasicBlock *, std::set<Value *>> flive_in1, flive_out1;
        std::map<BasicBlock *, std::set<Value *>> ilive_in1, ilive_out1;
};