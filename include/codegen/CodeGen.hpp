#pragma once

#include "ASMInstruction.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Module.hpp"
#include "Register.hpp"
#include "Value.hpp"
#include <string>
#include <unordered_map>
#include "RegAlloc.hpp"

class CodeGen {
  public:
    explicit CodeGen(Module *module) : m(module) {}

    std::string print() const;

    void run();

    template <class... Args> void append_inst(Args... arg) {
        output.emplace_back(arg...);
    }

    void
    append_inst(const char *inst, std::initializer_list<std::string> args,
                ASMInstruction::InstType ty = ASMInstruction::Instruction) {
        auto content = std::string(inst) + " ";
        for (const auto &arg : args) {
            content += arg + ", ";
        }
        content.pop_back();
        content.pop_back();
        output.emplace_back(content, ty);
    }

  private:
    void allocate();

    // 向寄存器中装载数据
    void load_to_greg(Value *, const Reg &);
    void load_to_freg(Value *, const FReg &);
    void load_from_stack_to_greg(Value *, const Reg &);

    // 向寄存器中加载立即数
    void load_large_int32(int32_t, const Reg &);
    void load_large_int64(int64_t, const Reg &);
    void load_float_imm(float, const FReg &);

    // 将寄存器中的数据保存回栈上
    void store_from_greg(Value *, const Reg &);
    void store_from_freg(Value *, const FReg &);

    std::pair<int,bool> get_reg(Value *);


    void gen_prologue();
    void gen_ret();
    void gen_br();
    void gen_binary();
    void gen_float_binary();
    void gen_alloca();
    void gen_load();
    void gen_store();
    void gen_icmp();
    void gen_fcmp();
    void gen_zext();
    void gen_call();
    void gen_gep();
    void gen_sitofp();
    void gen_fptosi();
    void gen_epilogue();

    static std::string label_name(BasicBlock *bb) {
        return "." + bb->get_parent()->get_name() + "_" + bb->get_name();
    }

    struct {
        /* 随着ir遍历设置 */
        Function *func{nullptr};    // 当前函数
        Instruction *inst{nullptr}; // 当前指令
        /* 在allocate()中设置 */
        unsigned frame_size{0}; // 当前函数的栈帧大小
        std::unordered_map<Value *, int> offset_map{}; // 指针相对 fp 的偏移

        void clear() {
            func = nullptr;
            inst = nullptr;
            frame_size = 0;
            offset_map.clear();
        }
    } context;
    std::map<std::pair<std::string,int >, int> arg_map; // 函数参数的位置
    Module *m;
    std::list<ASMInstruction> output;
    std::map<Value*, Interval*> ireg_alloc;
    std::map<Value*, Interval*> freg_alloc;
    std::map<int, Value*> ireg_map;
    std::map<int, Value*> freg_map;
    std::list<BasicBlock*> block_order;
    bool is_br = false;
    BasicBlock *br_bb = nullptr;
    int br_num = -1;
    std::map<int,Value*> ireg_map_false;
    std::map<int,Value*> freg_map_false;
    std::vector<std::pair<BasicBlock *, BasicBlock *>> br_map;
    std::map<BasicBlock *, BasicBlock *> br_false_map;
    std::map<BasicBlock *, std::map<int, Value *>> br_ireg_map;
    std::map<BasicBlock *, std::map<int, Value *>> br_freg_map;
};
