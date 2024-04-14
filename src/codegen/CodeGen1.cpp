#include "CodeGen.hpp"
#include "Constant.hpp"
#include "RegAlloc.hpp"
#include "ASMInstruction.hpp"
#include "BasicBlock.hpp"
#include "CodeGenUtil.hpp"
#include "GlobalVariable.hpp"
#include "Instruction.hpp"
#include "Register.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include <atomic>
#include <string>
#include <vector>

void CodeGen::allocate(){
    // 备份 $ra $fp
    unsigned offset = PROLOGUE_OFFSET_BASE;

     // 为每个参数分配栈空间
    for (auto &arg : context.func->get_args()) {
        auto size = arg.get_type()->get_size();
        offset = ALIGN(offset + size, size);
        context.offset_map[&arg] = -static_cast<int>(offset);
        arg_map[{context.func->get_name(), arg.get_arg_no()}] = -static_cast<int>(offset);
    }

    // 为指令结果分配栈空间
    for (auto &bb : context.func->get_basic_blocks()) {
        for (auto &instr : bb.get_instructions()) {
            // 每个非 void 的定值都分配栈空间
            if (not instr.is_void()) {
                auto size = instr.get_type()->get_size();
                if(instr.is_mul64())
                {
                    //LOG_DEBUG << "Allocate4582 " << instr.print() ;
                    size = 8;
                }
                offset = ALIGN(offset + size, size);
                context.offset_map[&instr] = -static_cast<int>(offset);
            }
            // alloca 的副作用：分配额外空间
            if (instr.is_alloca()) {
                auto *alloca_inst = static_cast<AllocaInst *>(&instr);
                auto alloc_size = alloca_inst->get_alloca_type()->get_size();
                offset += alloc_size;
            }
        }
    }

    // 分配栈空间，需要是 16 的整数倍
    context.frame_size = ALIGN(offset, PROLOGUE_ALIGN);
}
void CodeGen::gen_prologue() {
    // 寄存器备份及栈帧设置
    if (IS_IMM_12(-static_cast<int>(context.frame_size))) {
      //  LOG_DEBUG << "Prologue";
        append_inst("st.d $ra, $sp, -8");
        append_inst("st.d $fp, $sp, -16");
        append_inst("addi.d $fp, $sp, 0");
        append_inst("addi.d $sp, $sp, " +
                    std::to_string(-static_cast<int>(context.frame_size)));
    } else {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("st.d $ra, $sp, -8");
        append_inst("st.d $fp, $sp, -16");
        append_inst("sub.d $sp, $sp, $t0");
        append_inst("add.d $fp, $sp, $t0");
    }

    // 将函数参数转移到栈帧上
    int garg_cnt = 0;
    int farg_cnt = 0;
    for (auto &arg : context.func->get_args()) {
        if (arg.get_type()->is_float_type() && farg_cnt < 8) {
            store_from_freg(&arg, FReg::fa(farg_cnt++));
        } else if(arg.get_type()->is_float_type() && farg_cnt == 8){
            //append_inst("ld.d $t0, $fp, -16"); 
            //load_to_freg(&arg,FReg::ft(0));
        }
        else if(garg_cnt < 8){ // int or pointer
            store_from_greg(&arg, Reg::a(garg_cnt++));
        }
    }
   // LOG_DEBUG << "Prologue done";
}
void CodeGen::load_to_greg(Value *val, const Reg &reg) {
    assert(val->get_type()->is_integer_type() ||
           val->get_type()->is_pointer_type());

    if (auto *constant = dynamic_cast<ConstantInt *>(val)) {
        int32_t val = constant->get_value();
        if (IS_IMM_12(val)) {
            append_inst(ADDI WORD, {reg.print(), "$zero", std::to_string(val)});
        } else {
            load_large_int32(val, reg);
        }
    } else if (auto *global = dynamic_cast<GlobalVariable *>(val)) {
        append_inst(LOAD_ADDR, {reg.print(), global->get_name()});
    } else {
        load_from_stack_to_greg(val, reg);
    }
}

void CodeGen::load_large_int32(int32_t val, const Reg &reg) {
    int32_t high_20 = val >> 12; // si20
    uint32_t low_12 = val & LOW_12_MASK;
    append_inst(LU12I_W, {reg.print(), std::to_string(high_20)});
    append_inst(ORI, {reg.print(), reg.print(), std::to_string(low_12)});
}

void CodeGen::load_large_int64(int64_t val, const Reg &reg) {
    auto low_32 = static_cast<int32_t>(val & LOW_32_MASK);
    load_large_int32(low_32, reg);

    auto high_32 = static_cast<int32_t>(val >> 32);
    int32_t high_32_low_20 = (high_32 << 12) >> 12; // si20
    int32_t high_32_high_12 = high_32 >> 20;        // si12
    append_inst(LU32I_D, {reg.print(), std::to_string(high_32_low_20)});
    append_inst(LU52I_D,
                {reg.print(), reg.print(), std::to_string(high_32_high_12)});
}

void CodeGen::load_from_stack_to_greg(Value *val, const Reg &reg) {
    auto offset = context.offset_map.at(val);
    auto offset_str = std::to_string(offset);
    auto *type = val->get_type();
    if (IS_IMM_12(offset)) {
        if (type->is_int1_type()) {
            append_inst(LOAD BYTE, {reg.print(), "$fp", offset_str});
        } else if (type->is_int32_type() && (context.inst == nullptr || (!context.inst->is_ashr64() && !context.inst->is_lshr64()))) {
            append_inst(LOAD WORD, {reg.print(), "$fp", offset_str});
        } else { // Pointer
            append_inst(LOAD DOUBLE, {reg.print(), "$fp", offset_str});
        }
    } else {
        load_large_int64(offset, reg);
        append_inst(ADD DOUBLE, {reg.print(), "$fp", reg.print()});
        if (type->is_int1_type()) {
            append_inst(LOAD BYTE, {reg.print(), reg.print(), "0"});
        } else if (type->is_int32_type() && (context.inst == nullptr || (!context.inst->is_ashr64() && !context.inst->is_lshr64()))) {
            append_inst(LOAD WORD, {reg.print(), reg.print(), "0"});
        } else { // Pointer
            append_inst(LOAD DOUBLE, {reg.print(), reg.print(), "0"});
        }
    }
}

void CodeGen::store_from_greg(Value *val, const Reg &reg) {
    auto offset = context.offset_map.at(val);
    auto offset_str = std::to_string(offset);
    auto *type = val->get_type();
    if (IS_IMM_12(offset)) {
        if (type->is_int1_type()) {
            append_inst(STORE BYTE, {reg.print(), "$fp", offset_str});
        } else if (type->is_int32_type() && (context.inst == nullptr || !context.inst->is_mul64())){
            append_inst(STORE WORD, {reg.print(), "$fp", offset_str});
        } else { // Pointer
           // LOG_DEBUG << context.inst->is_mul64() << "      55555";
            append_inst(STORE DOUBLE, {reg.print(), "$fp", offset_str});
        }
    } else {
        auto addr = Reg::t(8);
        load_large_int64(offset, addr);
        append_inst(ADD DOUBLE, {addr.print(), "$fp", addr.print()});
        if (type->is_int1_type()) {
            append_inst(STORE BYTE, {reg.print(), addr.print(), "0"});
        } else if (type->is_int32_type() && (context.inst == nullptr || !context.inst->is_mul64())) {
            append_inst(STORE WORD, {reg.print(), addr.print(), "0"});
        } else { // Pointer
            //LOG_DEBUG << context.inst->is_mul64() << "      55555";
            append_inst(STORE DOUBLE, {reg.print(), addr.print(), "0"});
        }
    }
}

void CodeGen::load_to_freg(Value *val, const FReg &freg) {
    assert(val->get_type()->is_float_type());
    if (auto *constant = dynamic_cast<ConstantFP *>(val)) {
        float val = constant->get_value();
        load_float_imm(val, freg);
    } else {
        auto offset = context.offset_map.at(val);
        auto offset_str = std::to_string(offset);
        if (IS_IMM_12(offset)) {
            append_inst(FLOAD SINGLE, {freg.print(), "$fp", offset_str});
        } else {
            auto addr = Reg::t(8);
            load_large_int64(offset, addr);
            append_inst(ADD DOUBLE, {addr.print(), "$fp", addr.print()});
            append_inst(FLOAD SINGLE, {freg.print(), addr.print(), "0"});
        }
    }
}

void CodeGen::load_float_imm(float val, const FReg &r) {
    int32_t bytes = *reinterpret_cast<int32_t *>(&val);
    load_large_int32(bytes, Reg::t(8));
    append_inst(GR2FR WORD, {r.print(), Reg::t(8).print()});
}

void CodeGen::store_from_freg(Value *val, const FReg &r) {
    auto offset = context.offset_map.at(val);
    if (IS_IMM_12(offset)) {
        auto offset_str = std::to_string(offset);
        append_inst(FSTORE SINGLE, {r.print(), "$fp", offset_str});
    } else {
        auto addr = Reg::t(8);
        load_large_int64(offset, addr);
        append_inst(ADD DOUBLE, {addr.print(), "$fp", addr.print()});
        append_inst(FSTORE SINGLE, {r.print(), addr.print(), "0"});
    }
}

void CodeGen::gen_epilogue() {
    // TODO: 根据你的理解设定函数的 epilogue
    append_inst("epilogue code",ASMInstruction::Comment);
    append_inst(context.func->get_name() + "_exit", ASMInstruction::Label);
    if(IS_IMM_12(context.frame_size)){
        append_inst("addi.d $sp, $sp, " + std::to_string(context.frame_size));
        append_inst("ld.d $ra, $sp, -8");
        append_inst("ld.d $fp, $sp, -16");
        append_inst("jr $ra");
    }
    else
    {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("add.d $sp, $sp, $t0");
        append_inst("ld.d $ra, $sp, -8");
        append_inst("ld.d $fp, $sp, -16");
        append_inst("jr $ra");
    }
   // throw not_implemented_error{__FUNCTION__};
}

int flag = 0;
std::pair<int,bool> CodeGen::get_reg(Value *v)
{
    //auto reg = Reg::zero();
    auto v_cast = dynamic_cast<ConstantInt *>(v);
    auto v_cast_fp = dynamic_cast<ConstantFP *>(v);
    if(v_cast)
    {
        if(!flag)
        {
            flag = 1;
            return {20, false};
        }
        flag = 0;
        return {19, false};
    }
    else if(v_cast_fp)
    {
        if(!flag)
        {
            flag = 1;
            return {23, false};
        }
        flag = 0;
        return {22, false};
    }

    if(ireg_alloc.find(v) != ireg_alloc.end())
    {
        auto val = ireg_alloc[v];
        auto reg_id = val->reg_id;
        if(reg_id == -1)
        {
            if(!flag)
            {
                flag = 1;
                return {20, false};
            }
            flag = 0;
            return {19, false};
        }
        else 
        {
            auto ireg_map_find = ireg_map.find(reg_id);
            if(ireg_map_find != ireg_map.end())
            {
               if(ireg_map_find->second != v)
               {
                    //store_from_greg(ireg_map_find->second, Reg(reg_id));
                    ireg_map[reg_id] = v;
                    return {reg_id, false};
               }
               return {reg_id, true};
            }
            else 
            {
                ireg_map[reg_id] = v;
                LOG_DEBUG << "ireg_map " << reg_id << "  " << v->get_name();
                return {reg_id, false};
            }
            //return {reg_id, false};  
        }
    }
    else if(freg_alloc.find(v) != freg_alloc.end())
    {
        auto val = freg_alloc[v];
        auto reg_id = val->reg_id;
        if(reg_id == -1)
        {
            if(!flag)
            {
                flag = 1;
                return {23,false};
            }
            flag = 0;
            return {22,false};
        }
        else 
        {
            auto freg_map_find = freg_map.find(reg_id);
            if(freg_map_find != freg_map.end())
            {
                if(freg_map_find->second != v)
                {
                    freg_map[reg_id] = v;
                    return {reg_id, false};
                }
                return {reg_id, true};
            }
            else
            {
                freg_map[reg_id] = v;
                return {reg_id, false};
            }
            //return {reg_id, true};   
        }
    }
    else
    {
        return {-1,false};
    }
}

void CodeGen::gen_ret() {
   
    if(context.func->get_return_type()->is_void_type()) 
    {
        append_inst("addi.d $a0, $zero, 0",ASMInstruction::Instruction);
    }
    else if(context.func->get_return_type()->is_int32_type() || context.func->get_return_type()->is_pointer_type())
    {
        auto  op1_reg_id = get_reg(context.inst->get_operand(0));
        if(op1_reg_id.second == false)
            load_to_greg(context.inst->get_operand(0), Reg::a(0));
        else
        {
            auto reg_id = get_reg(context.inst->get_operand(0));
            if(reg_id.first != 4)
                append_inst("add.d $a0, $zero, " + Reg(reg_id.first).print(),ASMInstruction::Instruction);
        }
    }
    else if(context.func->get_return_type()->is_float_type())
    {
        auto  op1_reg_id = get_reg(context.inst->get_operand(0));
        if(op1_reg_id.second == false)
            load_to_freg(context.inst->get_operand(0), FReg::fa(0));
        else
        {
            auto reg_id = get_reg(context.inst->get_operand(0));
            if(reg_id.first != 0)
                append_inst("fadd.s $fa0, $zero, " + FReg(reg_id.first).print(),ASMInstruction::Instruction);
        }
    }
    append_inst("b " + context.func->get_name() + "_exit", ASMInstruction::Instruction);
}

void CodeGen::gen_br() {
    auto *branchInst = static_cast<BranchInst *>(context.inst);
    std::vector<Value *> target,source;
    std::vector<std::pair<Value *, Value *>> phis;
    if(branchInst->is_cond_br())
    {
        //auto  *truebb = static_cast<BasicBlock *>(branchInst->get_operand(1));
        //auto *falsebb = static_cast<BasicBlock *>(branchInst->get_operand(2));
        for(auto i = 1;i <= 2;i ++)
        {
            auto succ_bb = static_cast<BasicBlock *>(branchInst->get_operand(i));
            for(auto &instr : succ_bb->get_instructions())
            {
                auto instr1 = &instr;
                if(instr.is_phi())
                {
                  //  LOG_DEBUG << "phi instr" << instr.print() << branchInst->print();
                    Value *pre_op = nullptr;
                    for(auto op : instr.get_operands())
                    {
                        if(dynamic_cast<BasicBlock *>(op) 
                        && dynamic_cast<BasicBlock *>(op)->get_terminator() == branchInst)
                        {
                            phis.push_back({instr1,pre_op});
                        }
                        pre_op = op;
                    }
                }
            }
        }
    }
    else 
    {
        auto succ_bb = static_cast<BasicBlock *>(branchInst->get_operand(0));
        for(auto &instr : succ_bb->get_instructions())
        {
            auto instr1 = &instr;
            if(instr.is_phi())
            {
              //  LOG_DEBUG << "phi instr " << instr.print() <<"   " << branchInst->get_parent()->get_name() << "   "<< branchInst->print();
                Value *val = nullptr;
                for(auto op : instr.get_operands())
                {
                    if(dynamic_cast<BasicBlock *>(op) 
                    && dynamic_cast<BasicBlock *>(op)->get_terminator() == branchInst)
                    {
                        phis.push_back({instr1,val});
                    }
                    val = op;
                }
            }
        }
    }
    for(auto &pair: phis)
    {
        auto *phi = static_cast<PhiInst *>(pair.first);
        auto *val = pair.second;
        if(val->get_type()->is_float_type())
        {
            //load_to_freg(val, FReg::ft(0));
            auto val_reg_id = get_reg(val);
            auto phi_reg_id = get_reg(phi);
            if(dynamic_cast<ConstantFP *>(val))
            {
                if(phi_reg_id.first == 22 || phi_reg_id.first == 23)
                {
                    load_float_imm(dynamic_cast<ConstantFP *>(val)->get_value(), FReg(val_reg_id.first));
                    store_from_freg(phi, FReg::ft(val_reg_id.first));
                }
                else
                {
                    load_float_imm(dynamic_cast<ConstantFP *>(val)->get_value(), FReg(phi_reg_id.first));
                }
            }
            else
            {
                if(val_reg_id.second == false && (phi_reg_id.first == 22 || phi_reg_id.first == 23))
                {
                    load_to_freg(val, FReg(val_reg_id.first));
                    store_from_freg(phi, FReg(val_reg_id.first));
                }
                else if(val_reg_id.second == false && (phi_reg_id.first != 22 && phi_reg_id.first != 23))
                {
                    load_to_freg(val, FReg(phi_reg_id.first));
                }
                else if(val_reg_id.second == true && (phi_reg_id.first != 22 && phi_reg_id.first != 23))
                {
                    if(val_reg_id.first != phi_reg_id.first)
                    {
                        append_inst("fadd.d " + FReg(phi_reg_id.first).print() + ", " + FReg(val_reg_id.first).print() + ", $zero");
                    }
                }
                else
                {
                    store_from_freg(phi, FReg(val_reg_id.first));
                }
            }
        }
        else
        {
            auto val_reg_id = get_reg(val);
            auto phi_reg_id = get_reg(phi);
            if(dynamic_cast<ConstantInt *> (val))
            {
                if(phi_reg_id.first == 19 || phi_reg_id.first == 20)
                {
                    load_large_int32(dynamic_cast<ConstantInt *>(val)->get_value(), Reg(val_reg_id.first));
                    store_from_greg(phi, Reg(val_reg_id.first));
                }
                else
                {
                    load_large_int32(dynamic_cast<ConstantInt *>(val)->get_value(), Reg(phi_reg_id.first));
                }
            }
            else
            {
                if(val_reg_id.second == false && (phi_reg_id.first == 19 || phi_reg_id.first == 20))
                {
                    load_to_greg(val, Reg(val_reg_id.first));
                    store_from_greg(phi, Reg(val_reg_id.first));
                }
                else if(val_reg_id.second == false && (phi_reg_id.first != 19 && phi_reg_id.first != 20))
                {
                    load_to_greg(val, Reg(phi_reg_id.first));
                }
                else if(val_reg_id.second == true && (phi_reg_id.first != 19 && phi_reg_id.first != 20))
                {
                    if(val_reg_id.first != phi_reg_id.first)
                    {
                        append_inst("add.d " + Reg(phi_reg_id.first).print() + ", " + Reg(val_reg_id.first).print() + ", $zero");
                    }
                }
                else
                {
                    store_from_greg(phi, Reg(val_reg_id.first));
                }
            }
        }
    }


    if (branchInst->is_cond_br()) {
        // TODO: 补全条件跳转的情况
        if(branchInst->get_operand(0)->get_type()->is_int1_type())
        {
            auto fcmp = dynamic_cast<FCmpInst *>(branchInst->get_operand(0));
            if(fcmp != nullptr)
            {
                auto truebb = static_cast<BasicBlock *>(branchInst->get_operand(1));
                auto falsebb = static_cast<BasicBlock *>(branchInst->get_operand(2));
                append_inst("bnez $fcc0 ", {label_name(truebb)});
                append_inst("b ", {label_name(falsebb)});
                return;
            }
            auto reg_id = get_reg(branchInst->get_operand(0));
            if(reg_id.second == false)
            {
                load_to_greg(branchInst->get_operand(0), Reg(reg_id.first));
            }
            auto  *truebb = static_cast<BasicBlock *>(branchInst->get_operand(1));
            append_inst("bnez", {Reg(reg_id.first).print(), label_name(truebb)});
            auto *falsebb = static_cast<BasicBlock *>(branchInst->get_operand(2));
            append_inst("b " + label_name(falsebb));
        }
        //throw not_implemented_error{__FUNCTION__};
    } else {
        auto *branchbb = static_cast<BasicBlock *>(branchInst->get_operand(0));
        append_inst("b " + label_name(branchbb));
    }
}

void CodeGen::gen_binary()
{
    auto op1_reg_id = get_reg(context.inst->get_operand(0));
    auto op2_reg_id = get_reg(context.inst->get_operand(1));
    auto dest_reg_id = get_reg(context.inst);
    if(op1_reg_id.second == false)
    {
        load_to_greg(context.inst->get_operand(0), Reg(op1_reg_id.first));
    }
    if(op2_reg_id.second == false)
    {
        if(!context.inst->is_mul64())
            load_to_greg(context.inst->get_operand(1), Reg(op2_reg_id.first));
        else
        {
            auto constant = dynamic_cast<ConstantInt *>(context.inst->get_operand(1));
            // LOG_DEBUG << "gen_binary" << constant->get_value();
            load_large_int64(constant->get_value(),Reg::t(1));
        }
    }
    switch(context.inst->get_instr_type())
    {
        case Instruction::add:
            append_inst("add.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::sub:
            append_inst("sub.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::mul:
            append_inst("mul.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::sdiv:
            append_inst("div.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::srem:
            append_inst("mod.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::shl:
            append_inst("sll.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::ashr:
            append_inst("sra.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::lshr:
            append_inst("srl.w " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::land:
            append_inst("and " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::mul64:
            append_inst("mul.d " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::lshr64:
            append_inst("srl.d " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::ashr64:
            append_inst("sra.d " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        default:
            break;
    }
    if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
    {
        store_from_greg(context.inst, Reg(dest_reg_id.first));
    }
}

void CodeGen::gen_float_binary()
{
    auto op1_reg_id = get_reg(context.inst->get_operand(0));
    auto op2_reg_id = get_reg(context.inst->get_operand(1));
    auto dest_reg_id = get_reg(context.inst);
    if(op1_reg_id.second == false)
    {
        load_to_freg(context.inst->get_operand(0), FReg(op1_reg_id.first));
    }
    if(op2_reg_id.second == false)
    {
        load_to_freg(context.inst->get_operand(1), FReg(op2_reg_id.first));
    }
    switch(context.inst->get_instr_type())
    {
        case Instruction::fadd:
            append_inst("fadd.s " + FReg(dest_reg_id.first).print() + ", " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
            break;
        case Instruction::fsub:
            append_inst("fsub.s " + FReg(dest_reg_id.first).print() + ", " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
            break;
        case Instruction::fmul:
            append_inst("fmul.s " + FReg(dest_reg_id.first).print() + ", " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
            break;
        case Instruction::fdiv:
            append_inst("fdiv.s " + FReg(dest_reg_id.first).print() + ", " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
            break;
        default:
            break;
    }
    if(dest_reg_id.first == 23 || dest_reg_id.first == 22)
    {
        store_from_freg(context.inst, FReg(dest_reg_id.first));
    }
}

void CodeGen::gen_alloca()
{

}

void CodeGen::gen_load()
{
    auto ptr = context.inst->get_operand(0);
    auto type = context.inst->get_type();
    auto ptr_reg_id = get_reg(ptr);
    if(dynamic_cast<GlobalVariable *>(ptr))
    {
        ptr_reg_id = {19,true};
        load_to_greg(ptr, Reg(19));
    }
    auto dest_reg_id = get_reg(context.inst);
    if(ptr_reg_id.second == false)
    {
        load_to_greg(ptr, Reg(ptr_reg_id.first));
    }
    if(type->is_float_type())
    {
        append_inst("fld.s " + FReg(dest_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
        if(dest_reg_id.first == 23 || dest_reg_id.first == 22)
        {
            store_from_freg(context.inst, FReg(dest_reg_id.first));
        }
    }
    else 
    {
        if(type->is_int32_type())
        {    
            append_inst("ld.w " + Reg(dest_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
        }
        else if(type->is_int1_type())
        {
            append_inst("ld.b " + Reg(dest_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
        }
        else 
        {
            append_inst("ld.d " + Reg(dest_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
        }
        if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
        {
            store_from_greg(context.inst, Reg(dest_reg_id.first));
        }
    }
}
void CodeGen::gen_store()
{
    auto val = context.inst->get_operand(0);
    auto ptr = context.inst->get_operand(1);
    auto type = val->get_type();
    auto val_reg_id = get_reg(val);
    auto ptr_reg_id = get_reg(ptr);
    if(dynamic_cast<GlobalVariable *>(ptr))
    {
        if(val_reg_id.first == 20)
        {
            ptr_reg_id = {19,true};
            load_to_greg(ptr, Reg(19));
        }
        else
        {
            ptr_reg_id = {20,true};
            load_to_greg(ptr, Reg(20));
        }
    }
   
   // LOG_DEBUG << val_reg_id.first << "   " << ptr_reg_id.first;
    if(val_reg_id.second == false)
    {
        if(type->is_float_type())
        {
            load_to_freg(val, FReg(val_reg_id.first));
        }
        else 
        {
            load_to_greg(val, Reg(val_reg_id.first));
        }
    }
    if(ptr_reg_id.second == false)
    {
        load_to_greg(ptr, Reg(ptr_reg_id.first));
    }
    if(type->is_float_type())
    {
        append_inst("fst.s " + FReg(val_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
    }
    else 
    {
        if(type->is_int32_type())
        {
            append_inst("st.w " + Reg(val_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
        }
        else if(type->is_int1_type())
        {
            append_inst("st.b " + Reg(val_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
        }
        else 
        {
            append_inst("st.d " + Reg(val_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", 0");
        }
    }
}

void CodeGen::gen_icmp()
{
    auto op1_reg_id = get_reg(context.inst->get_operand(0));
    auto op2_reg_id = get_reg(context.inst->get_operand(1));
    auto dest_reg_id = get_reg(context.inst);
    if(op1_reg_id.second == false)
    {
        load_to_greg(context.inst->get_operand(0), Reg(op1_reg_id.first));
    }
    if(op2_reg_id.second == false)
    {
        load_to_greg(context.inst->get_operand(1), Reg(op2_reg_id.first));
    }
    switch(context.inst->get_instr_type())
    {
        case Instruction::eq:
            //append_inst("xor $t2, $t0, $t1");
            //append_inst("sltui $t2, $t2, 1");
            //store_from_greg(context.inst, Reg::t(2)); 
            append_inst("xor " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            append_inst("sltui " + Reg(dest_reg_id.first).print() + ", " + Reg(dest_reg_id.first).print() + ", 1");
            break;
        case Instruction::ne:
            //append_inst("xor $t2, $t0, $t1");
            //append_inst("sltu $t2, $zero, $t2");
            //store_from_greg(context.inst, Reg::t(2)); 
            append_inst("xor " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            append_inst("sltu " + Reg(dest_reg_id.first).print() + ", $zero, " + Reg(dest_reg_id.first).print());
            break;
        case Instruction::gt:
            //append_inst("slt $t2, $t1, $t0");
            //store_from_greg(context.inst, Reg::t(2)); 
            append_inst("slt " + Reg(dest_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print());
            break;
        case Instruction::ge:
            //append_inst("slt $t2, $t0, $t1");
            //append_inst("xori $t2, $t2, 1");
            //store_from_greg(context.inst, Reg::t(2)); 
            append_inst("slt " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            append_inst("xori " + Reg(dest_reg_id.first).print() + ", " + Reg(dest_reg_id.first).print() + ", 1");
            break;
        case Instruction::lt:
            //append_inst("slt $t2, $t0, $t1");
            //store_from_greg(context.inst, Reg::t(2)); 
            append_inst("slt " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print());
            break;
        case Instruction::le:
            //append_inst("slt $t2, $t1, $t0");
            //append_inst("xori $t2, $t2, 1");
            //store_from_greg(context.inst, Reg::t(2)); 
            append_inst("slt " + Reg(dest_reg_id.first).print() + ", " + Reg(op2_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print());
            append_inst("xori " + Reg(dest_reg_id.first).print() + ", " + Reg(dest_reg_id.first).print() + ", 1");
            break;
        default:
            assert(false);
    }
    if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
    {
        store_from_greg(context.inst, Reg(dest_reg_id.first));
    }
}

int feq_counter = 0;
int fne_counter = 0;
int fgt_counter = 0;
int fge_counter = 0;
int flt_counter = 0;
int fle_counter = 0;
void CodeGen::gen_fcmp()
{
    auto op1_reg_id = get_reg(context.inst->get_operand(0));
    auto op2_reg_id = get_reg(context.inst->get_operand(1));
    //auto dest_reg_id = get_reg(context.inst);
    if(op1_reg_id.second == false)
    {
        load_to_freg(context.inst->get_operand(0), FReg(op1_reg_id.first));
    }
    if(op2_reg_id.second == false)
    {
        load_to_freg(context.inst->get_operand(1), FReg(op2_reg_id.first));
    }
    switch(context.inst->get_instr_type())
    {
        case Instruction::feq:
             append_inst("fcmp.seq.s $fcc0 " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
        case Instruction::fne:
            append_inst("fcmp.sne.s $fcc0 " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
        case Instruction::fgt:
            append_inst("fcmp.sgt.s $fcc0 " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
        case Instruction::fge:
            append_inst("fcmp.sge.s $fcc0 " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
        case Instruction::flt:
            append_inst("fcmp.slt.s $fcc0 " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
        case Instruction::fle:
            append_inst("fcmp.sle.s $fcc0 " + FReg(op1_reg_id.first).print() + ", " + FReg(op2_reg_id.first).print());
    }
}
void CodeGen::gen_zext()
{
    auto op1_reg_id = get_reg(context.inst->get_operand(0));
    auto type = context.inst->get_type();
    auto dest_reg_id = get_reg(context.inst);
    if(op1_reg_id.second == false)
    {
        load_to_greg(context.inst->get_operand(0), Reg(op1_reg_id.first));
    }
    append_inst("bstrpick.d " + Reg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print() + ", 0, 0");
    if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
    {
        store_from_greg(context.inst, Reg(dest_reg_id.first));
    }
}

void CodeGen::gen_call()
{
    auto *func = context.inst->get_operand(0);
    auto *callee = static_cast<Function *>(func);
    int m = 0,n = 0;  
    for(auto &freg:freg_map)
    {
        LOG_DEBUG << freg.first << " " << freg.second->get_name();
        if(freg.second != nullptr)
        {
            store_from_freg(freg.second, FReg(freg.first));
        }
    }
    for(auto &greg:ireg_map)
    {
        LOG_DEBUG << greg.first << " " << greg.second->get_name();
        if(greg.second != nullptr)
        {
            store_from_greg(greg.second, Reg(greg.first));
        }
    }
    std::map<int,Value *> freg_map1;
    std::map<int,Value *> ireg_map1;
    freg_map1 = freg_map;
    ireg_map1 = ireg_map;
    freg_map.clear();
    ireg_map.clear();
    LOG_DEBUG << "gen_call" << callee->get_name();
    for(int i = 1;i <= callee->get_num_of_args();i ++)
    {
        auto arg = context.inst->get_operand(i);
        if(arg->get_type()->is_float_type() && m < 8)
        {    
            load_to_freg(arg, FReg::fa(m));
            m ++;
        }
        else if(arg->get_type()->is_float_type() && m >= 8)
        {
            load_to_freg(arg, FReg::ft(0));
         //   LOG_DEBUG<< callee->get_name() <<" "<< i << " " << arg_map[{callee->get_name(),i-1}];
            if(IS_IMM_12(arg_map[{callee->get_name(),i-1}]))
                append_inst("fst.s $ft0, $sp, " + std::to_string(arg_map[{callee->get_name(),i-1}]));
            else
            {
                load_large_int64(arg_map[{callee->get_name(),i-1}], Reg::t(0));
                append_inst("add.d $t0, $sp, $t0");
                append_inst("fst.s $ft0, $t0, 0");
            }
            m ++;
        }
        else if((arg->get_type()->is_integer_type() || arg->get_type()->is_pointer_type()) && n < 8)
        {
            LOG_DEBUG << arg->get_name() << context.offset_map[arg];
            load_to_greg(arg,Reg::a(n));
            n ++;
        }
        else 
        {
            load_to_greg(arg,Reg::t(0));
       //     LOG_DEBUG<<callee->get_name()<<" "<< i << " " << arg_map[{callee->get_name(),i-1}];   
            if(arg->get_type()->get_size() == 4)
            {
                if(IS_IMM_12(arg_map[{callee->get_name(),i-1}]))
                    append_inst("st.w $t0, $sp, " + std::to_string(arg_map[{callee->get_name(),i-1}]));
                else
                {
                    load_large_int64(arg_map[{callee->get_name(),i-1}], Reg::t(1));
                    append_inst("add.d $t0, $sp, $t1");
                    append_inst("st.w $t0, $t1, 0");
                }
            }
            else
            {
                if(IS_IMM_12(arg_map[{callee->get_name(),i-1}]))
                    append_inst("st.d $t0, $sp, " + std::to_string(arg_map[{callee->get_name(),i-1}]));
                else
                {
                    load_large_int64(arg_map[{callee->get_name(),i-1}], Reg::t(1));
                    append_inst("add.d $t0, $sp, $t1");
                    append_inst("st.d $t0, $t1, 0");
                }
            }
            n ++;
        }
    }
    append_inst("bl " + callee->get_name());
    auto dest_reg_id = get_reg(context.inst);
    if(!callee->get_return_type()->is_void_type())
    {
        if(callee->get_return_type()->is_float_type())
        {
            if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
                store_from_freg(context.inst, FReg::fa(0));
            else{
                if(dest_reg_id.first != 0) 
                    append_inst("fadd.d " + FReg(dest_reg_id.first).print() + ", $fa0, $zero");
            }
        }
        else
        {
            if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
                store_from_greg(context.inst, Reg::a(0));
            else
            {
                if(dest_reg_id.first != 4)
                    append_inst("add.d " + Reg(dest_reg_id.first).print() + ", $a0, $zero");
            }
        }
    }
    for(auto &freg:freg_map1)
    {
        if(callee->get_return_type()->is_float_type())
        {
            if(freg.first != dest_reg_id.first)
            {
                freg_map[freg.first] = freg.second;
                load_to_freg(freg.second, FReg(freg.first));
            }
        }
    }
    for(auto &greg:ireg_map1)
    {
        if(!callee->get_return_type()->is_float_type())
        {
            if(greg.first != dest_reg_id.first)
            {
                ireg_map[greg.first] = greg.second;
                load_to_greg(greg.second, Reg(greg.first));
            }
        }
    }
}

void CodeGen::gen_gep()
{
    auto ptr = context.inst->get_operand(0);
    auto dest_reg_id = get_reg(context.inst);
    Value *index = nullptr;
    if(context.inst->get_num_operand() == 3)
    {
        index = context.inst->get_operand(2);
    }
    else
    {
        index = context.inst->get_operand(1);
    }

    auto alloca = dynamic_cast<AllocaInst *>(ptr);
    auto ptr_reg_id = get_reg(ptr);
    if(alloca)
    {
        auto alloca_size = alloca->get_alloca_type()->get_size();
        auto offset = (int)(context.offset_map.at(ptr) - alloca_size);
        if(IS_IMM_12(offset))
            append_inst(ADDI DOUBLE, {Reg::t(8).print(), "$fp", std::to_string(offset)});
        else
        {
            load_large_int64(offset, Reg::t(8));
            append_inst(ADD DOUBLE, {Reg::t(8).print(), "$fp", Reg::t(8).print()});
        }
        ptr_reg_id = {20,true};
    }
    else 
    {
        if(ptr_reg_id.first == -1)
        {
            ptr_reg_id = {20,true};
            load_to_greg(ptr, Reg(20));
        }
        else  if(ptr_reg_id.second == false)
        {
            load_to_greg(ptr, Reg(ptr_reg_id.first));
        }
    }

    auto index_reg_id = get_reg(index);
    auto index_const = dynamic_cast<ConstantInt *>(index);
    if(index_const)
    {
        auto offset = index_const->get_value();
        if(IS_IMM_12(offset))
            append_inst(ADDI DOUBLE, {Reg(dest_reg_id.first).print(), Reg(ptr_reg_id.first).print(), std::to_string(offset*4)});
        else
        {
            load_large_int64(offset*4, Reg(index_reg_id.first));
            append_inst(ADD DOUBLE, {Reg(dest_reg_id.first).print(), Reg(ptr_reg_id.first).print(), Reg(index_reg_id.first).print()});
        }
    }
    else
    {
        if(index_reg_id.second == false)
        {
            load_to_greg(index, Reg(index_reg_id.first));
        }
        if(ptr_reg_id.first == 20)
        {    
            append_inst("slli.d " + Reg(19).print() + ", " + Reg(index_reg_id.first).print() + ", 2");
            append_inst("add.d " + Reg(dest_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", " + Reg(19).print());
        }
        else
        {
            append_inst("slli.d " + Reg(20).print() + ", " + Reg(index_reg_id.first).print() + ", 2");
            append_inst("add.d " + Reg(dest_reg_id.first).print() + ", " + Reg(ptr_reg_id.first).print() + ", " + Reg(20).print());
        }
    }

  
    if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
    {
        store_from_greg(context.inst, Reg(dest_reg_id.first));
    }
}

void CodeGen::gen_sitofp()
{
    auto op1_reg_id = get_reg(context.inst->get_operand(0));
    if(op1_reg_id.second == false)
    {
        load_to_greg(context.inst->get_operand(0), Reg(op1_reg_id.first));
    }
    auto dest_reg_id = get_reg(context.inst);
    append_inst("movgr2fr.w " + FReg(dest_reg_id.first).print() + ", " + Reg(op1_reg_id.first).print());
    append_inst("ffint.s.w " + FReg(dest_reg_id.first).print() + ", " + FReg(dest_reg_id.first).print());
    if(dest_reg_id.first == 23 || dest_reg_id.first == 22)
    {
        store_from_freg(context.inst, FReg(dest_reg_id.first));
    }
}

void CodeGen::gen_fptosi()
{
    auto op1_reg_id = get_reg(context.inst->get_operand(0));
    if(op1_reg_id.second == false)
    {
        load_to_freg(context.inst->get_operand(0), FReg(op1_reg_id.first));
    }
    auto dest_reg_id = get_reg(context.inst);
    append_inst("ftintz.w.s " + FReg(op1_reg_id.first).print() + ", " + FReg(op1_reg_id.first).print());
    append_inst("movfr2gr.s " + Reg(dest_reg_id.first).print() + ", " + FReg(op1_reg_id.first).print());
    if(dest_reg_id.first == 20 || dest_reg_id.first == 19)
    {
        store_from_greg(context.inst, Reg(dest_reg_id.first));
    }
}


void CodeGen::run() {
    // 确保每个函数中基本块的名字都被设置好
    // 想一想：为什么？
    m->set_print_name();

    /* 使用 GNU 伪指令为全局变量分配空间
     * 你可以使用 `la.local` 指令将标签 (全局变量) 的地址载入寄存器中, 比如
     * 要将 `a` 的地址载入 $t0, 只需要 `la.local $t0, a`
     */
    if (!m->get_global_variable().empty()) {
        append_inst("Global variables", ASMInstruction::Comment);
        /* 虽然下面两条伪指令可以简化为一条 `.bss` 伪指令, 但是我们还是选择使用
         * `.section` 将全局变量放到可执行文件的 BSS 段, 原因如下:
         * - 尽可能对齐交叉编译器 loongarch64-unknown-linux-gnu-gcc 的行为
         * - 支持更旧版本的 GNU 汇编器, 因为 `.bss` 伪指令是应该相对较新的指令,
         *   GNU 汇编器在 2023 年 2 月的 2.37 版本才将其引入
         */
        append_inst(".text", ASMInstruction::Atrribute);
        append_inst(".section", {".bss", "\"aw\"", "@nobits"},
                    ASMInstruction::Atrribute);
        std::vector<GlobalVariable *> global_vars;
        for (auto &global : m->get_global_variable()) {
            if(global.is_zero_init())
            {
            auto size =
                global.get_type()->get_pointer_element_type()->get_size();
            append_inst(".globl", {global.get_name()},
                        ASMInstruction::Atrribute);
            append_inst(".type", {global.get_name(), "@object"},
                        ASMInstruction::Atrribute);
            append_inst(".size", {global.get_name(), std::to_string(size)},
                        ASMInstruction::Atrribute);
            append_inst(global.get_name(), ASMInstruction::Label);
            append_inst(".space", {std::to_string(size)},
                        ASMInstruction::Atrribute);
            }
            else 
            {
                global_vars.push_back(&global);
            }
        }
        if(!global_vars.empty())
             append_inst(".section", {".data", "\"aw\"", "@progbits"},
                    ASMInstruction::Atrribute);
        for(auto &global : global_vars)
        {
            
            auto size =
                global->get_type()->get_pointer_element_type()->get_size();
            append_inst(".globl", {global->get_name()},
                        ASMInstruction::Atrribute);
            append_inst(".type", {global->get_name(), "@object"},
                        ASMInstruction::Atrribute);
            append_inst(".size", {global->get_name(), std::to_string(size)},
                        ASMInstruction::Atrribute);
            append_inst(global->get_name(), ASMInstruction::Label);
            //append_inst(".space", {std::to_string(size)},
             //           ASMInstruction::Atrribute);
            auto init_val = global->get_init();
            auto init_val_int = dynamic_cast<ConstantInt *>(init_val);
            auto init_val_fp = dynamic_cast<ConstantFP *>(init_val);
            auto init_val_array = dynamic_cast<ConstantArray *>(init_val);
            if(init_val_int)
            {
                append_inst(".word", {std::to_string(init_val_int->get_value())},
                    ASMInstruction::Atrribute);
            }
            else if(init_val_fp)
            {
                append_inst(".word", {std::to_string(init_val_fp->get_value())},
                    ASMInstruction::Atrribute);
            }
            else 
            {
                auto type = init_val_array->get_element_value(0)->get_type();
                for(auto i = 0;i < init_val_array->get_size_of_array();i ++)
                {
                    if(type->is_int32_type())
                    {
                        auto val = dynamic_cast<ConstantInt *>(init_val_array->get_element_value(i));
                        append_inst(".word", {std::to_string(val->get_value())},
                            ASMInstruction::Atrribute);
                    }
                    else if(type->is_float_type())
                    {
                        auto val = dynamic_cast<ConstantFP *>(init_val_array->get_element_value(i));
                        append_inst(".word", {std::to_string(val->get_value())},
                            ASMInstruction::Atrribute);
                    }
                }
            }
        }
    }
    auto reg_alloc_driver = new  RegAllocDriver(m);
    reg_alloc_driver->compute_reg_alloc();

    // 函数代码段
    output.emplace_back(".text", ASMInstruction::Atrribute);
    for (auto &func : m->get_functions()) {
        if (not func.is_declaration()) {
            // 更新 context
            context.clear();
            context.func = &func;

            ireg_alloc = reg_alloc_driver->get_ireg_alloc_in_func(&func);
            freg_alloc = reg_alloc_driver->get_freg_alloc_in_func(&func);
            ireg_map.clear();
            freg_map.clear();

            // 函数信息
            append_inst(".globl", {func.get_name()}, ASMInstruction::Atrribute);
            append_inst(".type", {func.get_name(), "@function"},
                        ASMInstruction::Atrribute);
            append_inst(func.get_name(), ASMInstruction::Label);

            // 分配函数栈帧
            allocate();
         //   LOG_DEBUG << "prologue begin";
            // 生成 prologue
            gen_prologue();
            block_order = reg_alloc_driver->get_block_order_in_func(&func);
        //    LOG_DEBUG << "Generating function: " << func.get_name();
            for (auto &bb : reg_alloc_driver->get_block_order_in_func(&func)) 
            {
                append_inst(label_name(bb), ASMInstruction::Label);
                if(br_ireg_map.find(bb) != br_ireg_map.end())
                {
                    LOG_DEBUG << bb->get_name();
                    ireg_map = br_ireg_map[bb];
                }
                if(br_freg_map.find(bb) != br_freg_map.end())
                {
                    LOG_DEBUG << bb->get_name();
                    freg_map = br_freg_map[bb];
                }
                auto &pre_bb1 = bb->get_pre_basic_blocks().back();
                auto &pre_bb2 = bb->get_pre_basic_blocks().front();
                if(br_ireg_map.find(pre_bb1) != br_ireg_map.end() && br_ireg_map.find(pre_bb2) != br_ireg_map.end())
                {
                    for(auto &br_ireg_map1:br_ireg_map[pre_bb1])
                    {
                        if(br_ireg_map1.second != nullptr)
                        {
                            ireg_map[br_ireg_map1.first] = br_ireg_map1.second;
                        }
                    }
                    for(auto &br_ireg_map2:br_ireg_map[pre_bb2])
                    {
                        if(br_ireg_map2.second != nullptr)
                        {
                            ireg_map[br_ireg_map2.first] = br_ireg_map2.second;
                        }
                    }
                }
                if(br_freg_map.find(pre_bb1) != br_freg_map.end() && br_freg_map.find(pre_bb2) != br_freg_map.end())
                {
                    for(auto &br_freg_map1:br_freg_map[pre_bb1])
                    {
                        if(br_freg_map1.second != nullptr)
                        {
                            freg_map[br_freg_map1.first] = br_freg_map1.second;
                        }
                    }
                    for(auto &br_freg_map2:br_freg_map[pre_bb2])
                    {
                        if(br_freg_map2.second != nullptr)
                        {
                            freg_map[br_freg_map2.first] = br_freg_map2.second;
                        }
                    }
                }
                for (auto &instr : bb->get_instructions()) 
                {
                    // For debug
                    if(!instr.is_mul64() && !instr.is_ashr64() && !instr.is_lshr64())
                        append_inst(instr.print(), ASMInstruction::Comment);
                    LOG_DEBUG << instr.print();
                    context.inst = &instr; // 更新 context
                    switch (instr.get_instr_type()) {
                    case Instruction::ret:
                        gen_ret();
                        break;
                    case Instruction::br:
                        gen_br();
                        if(dynamic_cast<BranchInst *>(&instr)->is_cond_br())
                        {
                            auto true_bb = dynamic_cast<BasicBlock *>(instr.get_operand(1));
                            auto false_bb = dynamic_cast<BasicBlock *>(instr.get_operand(2));
                            if(true_bb->get_name().find("true") != true_bb->get_name().npos)
                            { 
                                LOG_DEBUG << true_bb->get_name() << " " << false_bb->get_name();
                                br_map.push_back({true_bb,false_bb});
                                br_ireg_map[true_bb] = ireg_map;
                                br_freg_map[true_bb] = freg_map;
                                br_ireg_map[false_bb] = ireg_map;
                                br_freg_map[false_bb] = freg_map;
                            }
                            else if(true_bb->get_name().find("cond") != true_bb->get_name().npos)
                            {
                                LOG_DEBUG << true_bb->get_name() << " " << false_bb->get_name();
                                br_false_map[false_bb] = true_bb;
                                br_ireg_map[true_bb] = ireg_map;
                                br_freg_map[true_bb] = freg_map;
                            }
                        }
                        break;
                    case Instruction::add:
                    case Instruction::sub:
                    case Instruction::mul:
                    case Instruction::sdiv:
                    case Instruction::srem:
                    case Instruction::shl:
                    case Instruction::ashr:
                    case Instruction::lshr:
                    case Instruction::ashr64:
                    case Instruction::lshr64:
                    case Instruction::mul64:
                    case Instruction::land:
                        gen_binary();
                        break;
                    case Instruction::fadd:
                    case Instruction::fsub:
                    case Instruction::fmul:
                    case Instruction::fdiv:
                        gen_float_binary();
                        break;
                    case Instruction::alloca:
                        gen_alloca();
                        break;
                    case Instruction::load:
                        gen_load();
                        break;
                    case Instruction::store:
                        gen_store();
                        break;
                    case Instruction::ge:
                    case Instruction::gt:
                    case Instruction::le:
                    case Instruction::lt:
                    case Instruction::eq:
                    case Instruction::ne:
                        gen_icmp();
                        break;
                    case Instruction::fge:
                    case Instruction::fgt:
                    case Instruction::fle:
                    case Instruction::flt:
                    case Instruction::feq:
                    case Instruction::fne:
                        gen_fcmp();
                        break;
                    case Instruction::phi:
                        //throw not_implemented_error{"need to handle phi!"};
                        break;
                    case Instruction::call:
                        gen_call();
                        break;
                    case Instruction::getelementptr:
                        gen_gep();
                        break;
                    case Instruction::zext:
                        gen_zext();
                        break;
                    case Instruction::fptosi:
                        gen_fptosi();
                        break;
                    case Instruction::sitofp:
                        gen_sitofp();
                        break;
                    }
                }
            // 生成 epilogue
            }
            gen_epilogue();
            
        }
    }
}

std::string CodeGen::print() const {
    std::string result;
    for (const auto &inst : output) {
        result += inst.format();
    }
    return result;
}


