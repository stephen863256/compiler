#include "StrengthReduction.hpp"
#include "Constant.hpp"
#include "Instruction.hpp"
#include "logging.hpp"
#include <cmath>
#include <cstdint>

void StrengthReduction::run(){
   // LOG_DEBUG << "StrengthReduction::run()";
    for(auto &f : module_->get_functions()){
        auto func = &f;
        if(func->is_declaration()) continue;
       // LOG_DEBUG << "func: " << func->get_name();
        strength_reduction(func);
    }
}

void StrengthReduction::strength_reduction(Function *f){
   
    for(auto &bb : f->get_basic_blocks()){
        std::list <Instruction *> const_list;
        auto terminator = bb.get_terminator();
        for(auto &inst : bb.get_instructions()){
         //   LOG_DEBUG << "inst: " << inst.print();
            if(!inst.is_int_binary()) continue;
         //   LOG_DEBUG << "inst: " << inst.print();
            auto op1 = inst.get_operand(0);
            auto op2 = inst.get_operand(1);
            if(inst.is_mul())
            {
                auto op1_const = dynamic_cast<ConstantInt *>(op1);
                auto op2_const = dynamic_cast<ConstantInt *>(op2);
                if(op1_const && op1_const->get_value() == 1){
                    inst.replace_all_use_with(op2);
                    //const_list.push_back(&inst);
                }
                else if(op1_const && op1_const->get_value() == 0){
                    inst.replace_all_use_with(op1);
                    //const_list.push_back(&inst);
                }
                else if(op2_const && op2_const->get_value() == 1){
                    inst.replace_all_use_with(op1);
                    //const_list.push_back(&inst);
                }
                else if(op2_const && op2_const->get_value() == 0){
                    inst.replace_all_use_with(op2);
                    //const_list.push_back(&inst);
                }
                else if(op1_const)
                {
                    auto val = op1_const->get_value();
                    if((val & (val - 1)) == 0)
                        const_list.push_back(&inst);
                }
                else if(op2_const)
                {
                    auto val = op2_const->get_value();
                    if((val & (val - 1)) == 0)
                        const_list.push_back(&inst);
                }
            }
            else if(inst.is_rem())
            {
                auto op2_const = dynamic_cast<ConstantInt *>(op2);
                if(op2_const && op2_const->get_value() == 1){
                    inst.replace_all_use_with(ConstantInt::get(0, module_));
                }
                else
                    const_list.push_back(&inst);
            }
            else if(inst.is_div())
            {
                auto op2_const = dynamic_cast<ConstantInt *>(op2);
                if(op2_const && op2_const->get_value() == 1){
                    inst.replace_all_use_with(op1);
                }
                else
                    const_list.push_back(&inst);
            }
        }
        if(const_list.empty()) continue;
        bb.get_instructions().remove(terminator);
        for(auto &inst : const_list){
        auto op1 = inst->get_operand(0);
        auto op2 = inst->get_operand(1);
        if(inst->is_mul())
        {
            auto op1_const = dynamic_cast<ConstantInt *>(op1);
            auto op2_const = dynamic_cast<ConstantInt *>(op2);
            if(op1_const)
            {
                auto val = op1_const->get_value();
                if(val > 0)
                {
                    if((val & (val - 1)) == 0)
                    {
                        auto log_val = 0;
                        while(val > 1)
                        {
                            val >>= 1;
                            log_val++;
                        }
                        auto new_inst = IBinaryInst::create_shl(op2,ConstantInt::get(log_val,module_) ,inst->get_parent());
                        inst->replace_all_use_with(new_inst);
                        inst->get_parent()->insert_instruction(inst,new_inst);
                    }
                }
                else if(val < 0)
                {
                    auto new_val = -val;
                    if((new_val & (new_val - 1)) == 0)
                    {
                        auto log_val = 0;
                        while(new_val > 1)
                        {
                            new_val >>= 1;
                            log_val++;
                        }
                        auto new_inst = IBinaryInst::create_shl(op2,ConstantInt::get(log_val,module_) ,inst->get_parent());
                        inst->get_parent()->insert_instruction(inst,new_inst);
                        auto new_inst2 = IBinaryInst::create_sub(ConstantInt::get(0,module_),new_inst,inst->get_parent());
                        inst->replace_all_use_with(new_inst2);
                        inst->get_parent()->insert_instruction(new_inst,new_inst2);
                    }
                }    
            }
            else if(op2_const)
            {
                auto val = op2_const->get_value();
                if(val > 0)
                {
                    if((val & (val - 1)) == 0)
                    {
                        auto log_val = 0;
                        while(val > 1)
                        {
                            val >>= 1;
                            log_val++;
                        }
                        auto new_inst = IBinaryInst::create_shl(op1,ConstantInt::get(log_val,module_) ,inst->get_parent());
                        inst->replace_all_use_with(new_inst);
                        inst->get_parent()->insert_instruction(inst,new_inst);
                    }
                }
                else if(val < 0)
                {
                    if(val == -1)
                    {
                        auto new_inst = IBinaryInst::create_sub(ConstantInt::get(0,module_),op1,inst->get_parent());
                        inst->replace_all_use_with(new_inst);
                        inst->get_parent()->insert_instruction(inst,new_inst);
                        continue;
                    }
                    auto new_val = -val;
                    if((new_val & (new_val - 1)) == 0)
                    {
                        auto log_val = 0;
                        while(new_val > 1)
                        {
                            new_val >>= 1;
                            log_val++;
                        }
                        auto new_inst = IBinaryInst::create_shl(op1,ConstantInt::get(log_val,module_) ,inst->get_parent());
                        auto new_inst2 = IBinaryInst::create_sub(ConstantInt::get(0,module_),new_inst,inst->get_parent());
                        inst->replace_all_use_with(new_inst2);
                        inst->get_parent()->insert_instruction(inst,new_inst);
                        inst->get_parent()->insert_instruction(new_inst,new_inst2);
                    }
                }
            }
        }
        else if(inst->is_rem())
        {
            auto op2_const = dynamic_cast<ConstantInt *>(op2);
            if(op2_const)
            {
                auto val = op2_const->get_value();
                if((val & (val - 1)) == 0)
                {
                    // dest = srem lhs, 2^k
                    // ->
                    // t0 = sra lhs, 31
                    // t1 = srl t0, 32 - k
                    // t2 = add lhs, t1
                    // t3 = and t2, 2^k - 1
                    // dest = sub t3, t1 
                    auto asr = IBinaryInst::create_ashr(op1,ConstantInt::get(31,module_),inst->get_parent()); 
                    inst->get_parent()->insert_instruction(inst,asr);
                    int n = (int)(ceil(std::log2(val)))%32;
                  //  LOG_DEBUG << "23456val: " << val << "n: " << n;
                    auto lsr = IBinaryInst::create_lshr(asr,ConstantInt::get((32-n),module_),inst->get_parent());
                    inst->get_parent()->insert_instruction(asr,lsr);
                    auto add = IBinaryInst::create_add(op1,lsr,inst->get_parent());
                    inst->get_parent()->insert_instruction(lsr,add);
                    auto and_inst = IBinaryInst::create_and(add,ConstantInt::get(val-1,module_),inst->get_parent());
                    inst->get_parent()->insert_instruction(add,and_inst);
                    auto sub = IBinaryInst::create_sub(and_inst,lsr,inst->get_parent());
                    inst->replace_all_use_with(sub);
                    inst->get_parent()->insert_instruction(and_inst,sub);
                }
                else
                {
                    auto abs_val = std::abs(val);
                    int n = 32 + floor(std::log2(abs_val));
                    long long N = pow(2,n);
                    int64_t m = val > 0 ? (N/abs_val+1) : -(N/abs_val);
                  //  LOG_DEBUG << "12345m: " << m << "val: " << val << (val>0)<<" n: " << n << "abs_val: " << abs_val;
                    auto mul64 = IBinaryInst::create_mul64(op1,ConstantInt::get(m,module_),inst->get_parent());
                    inst->get_parent()->insert_instruction(inst,mul64);
                    auto asr = IBinaryInst::create_ashr64(mul64,ConstantInt::get(n,module_),inst->get_parent());
                    inst->get_parent()->insert_instruction(mul64,asr);
                    auto lsr = IBinaryInst::create_lshr64(mul64,ConstantInt::get(63,module_),inst->get_parent());
                    inst->get_parent()->insert_instruction(asr,lsr);
                    auto add = IBinaryInst::create_add(asr,lsr,inst->get_parent());
                    inst->get_parent()->insert_instruction(lsr,add);
                    //auto div = IBinaryInst::create_sdiv(op1,op2,inst->get_parent());
                    //inst->get_parent()->insert_instruction(inst,add);
                    auto mul = IBinaryInst::create_mul(add,op2,inst->get_parent());
                    inst->get_parent()->insert_instruction(add,mul);
                    auto sub = IBinaryInst::create_sub(op1,mul,inst->get_parent());
                    inst->replace_all_use_with(sub);
                    inst->get_parent()->insert_instruction(mul,sub);
                }
            }
        }
        else if(inst->is_div())
        {
            auto op2_const = dynamic_cast<ConstantInt *>(op2);
            if(op2_const)
            {
                int val = op2_const->get_value();
                
                    if((val & (val - 1)) == 0)
                    {
                        //auto add_inst = IBinaryInst::create_add(op1,ConstantInt::get(val-1,module_),inst->get_parent());
                        //inst->get_parent()->insert_instruction(inst,add_inst);
                      
                        // dest = div lhs, 2
                        // ->
                        // tmp = srl lhs, 31
                        // dest = add tmp, lhs
                        // dest = sra dest, 1
                        if(val == 2)
                        {
                            auto lsr = IBinaryInst::create_lshr(op1,ConstantInt::get(31,module_),inst->get_parent());
                          //  LOG_DEBUG << "lsr: " << lsr->print();
                            inst->get_parent()->insert_instruction(inst,lsr);
                            auto add = IBinaryInst::create_add(op1,lsr,inst->get_parent());
                            inst->get_parent()->insert_instruction(lsr,add);
                           // LOG_DEBUG << "add: " << add->print();
                            auto asr = IBinaryInst::create_ashr(add,ConstantInt::get(1,module_),inst->get_parent());
                            inst->get_parent()->insert_instruction(add,asr);
                          //  LOG_DEBUG << "asr: " << asr->print();
                            inst->replace_all_use_with(asr);
                        }
                        else
                        /*
                         if (x < 0)
                            x = (x+2^n-1)
                        x >> n
                        */
                        // dest = div lhs, 2^k
                        // ->
                        // t0 = sra lhs, 31
                        // t1 = srl t0, 32 - k
                        // t2 = add lhs, t1
                        // dest = sra t2, k
                       // if(val != 2)
                        {
                            auto asr = IBinaryInst::create_ashr(op1,ConstantInt::get(31,module_),inst->get_parent());
                            inst->get_parent()->insert_instruction(inst,asr);
                            int n = (int)(ceil(std::log2(val)))%32;
                            auto lsr = IBinaryInst::create_lshr(asr,ConstantInt::get((32-n),module_),inst->get_parent());
                            inst->get_parent()->insert_instruction(asr,lsr);
                            auto add = IBinaryInst::create_add(op1,lsr,inst->get_parent());
                            inst->get_parent()->insert_instruction(lsr,add);
                            auto asr2 = IBinaryInst::create_ashr(add,ConstantInt::get(n,module_),inst->get_parent());
                            inst->get_parent()->insert_instruction(add,asr2);
                            inst->replace_all_use_with(asr2);
                        }
                    }
                    else if(val == -1)
                    {
                        auto sub = IBinaryInst::create_sub(ConstantInt::get(0,module_),op1,inst->get_parent());
                        inst->get_parent()->insert_instruction(inst,sub);
                        inst->replace_all_use_with(sub);
                    }
                    else {
                        auto abs_val = std::abs(val);
                        int n = 32 + floor(std::log2(abs_val));
                        long long N = pow(2,n);
                        int64_t m = (val > 0) ? (N/abs_val+1) : -(N/abs_val);
                  //      LOG_DEBUG << "m: " << m << "val: " << val<<(val>0) << "n: " << n << "abs_val: " << abs_val;
                        auto const_m = ConstantInt::get(m,module_);
                       // LOG_DEBUG << "const_m: " << (int64_t)(const_m->get_value());
                        auto mul64 = IBinaryInst::create_mul64(op1,ConstantInt::get(m,module_),inst->get_parent());
                      //  LOG_DEBUG << "mul64: " << mul64->print();
                        inst->get_parent()->insert_instruction(inst,mul64);
                        auto asr = IBinaryInst::create_ashr64(mul64,ConstantInt::get(n,module_),inst->get_parent());
                        inst->get_parent()->insert_instruction(mul64,asr);
                        auto lsr = IBinaryInst::create_lshr64(mul64,ConstantInt::get(63,module_),inst->get_parent());
                        inst->get_parent()->insert_instruction(asr,lsr);
                        auto add = IBinaryInst::create_add(asr,lsr,inst->get_parent());
                        inst->get_parent()->insert_instruction(lsr,add);
                        inst->replace_all_use_with(add);
                    }
            }
        }
    }
    auto br_inst = dynamic_cast<BranchInst *>(terminator);
    if(br_inst)
    {
        if(br_inst->is_cond_br())
        {
            auto true_bb = dynamic_cast<BasicBlock *>(br_inst->get_operand(1));
            auto false_bb = dynamic_cast<BasicBlock *>(br_inst->get_operand(2));
            BranchInst::create_cond_br(br_inst->get_operand(0),true_bb,false_bb,br_inst->get_parent());
        }
        else
        {
            auto bb = dynamic_cast<BasicBlock *>(br_inst->get_operand(0));
            BranchInst::create_br(bb,br_inst->get_parent());
        }
    } 
    else
    {
        auto ret_inst = dynamic_cast<ReturnInst *>(terminator);
        if(ret_inst)
        {
            ReturnInst::create_ret(ret_inst->get_operand(0),ret_inst->get_parent());
        }
    }
    }
}