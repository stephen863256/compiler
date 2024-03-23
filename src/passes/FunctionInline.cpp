#include "FunctionInline.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include <vector>
#include "DeadCode.hpp"

void FunctionInline::run() {
    LOG_DEBUG << "FunctionInline run";
    inline_func_find();
    for(auto &call : call_list) {
        auto instr = call.first;
        auto func = call.second;
        //LOG_DEBUG <<"Inlining function: " + func->get_name();
        //LOG_DEBUG << "Instruction: " + instr->print();
    }
    inline_func();
    LOG_INFO << module_->print();
  //  std::shared_ptr<DeadCode> dead_code = std::make_shared<DeadCode>(module_);
   // dead_code->run();
}

void FunctionInline::inline_func_find() {
    for (auto &F : module_->get_functions()) {
        auto func = &F;
        if(func->is_declaration()) {
            continue;
        }
        if(func->get_name() == "main") {
            continue;
        }
        int num = 0;
        for(auto &bb : func->get_basic_blocks()) {
            num += bb.get_instructions().size();
        }
        if(num > MAX_INLINE) {
            continue;
        }
        if(func->get_num_basic_blocks() > 1) {
            continue;
        }
        for (auto &F : module_->get_functions()) {
            auto func_ = &F;
            if(func_->is_declaration()) {
                continue;
            }
            for(auto &bb : func_->get_basic_blocks()) {
                for(auto &instr : bb.get_instructions()) {
                    if(instr.is_call()) {
                        auto func_call = dynamic_cast<Function *>(instr.get_operand(0));
                        if(func_call == func) {
                            if(call_list.find({&instr, func}) == call_list.end())
                                call_list.insert({&instr, func});
                        }
                    }
                }
            }
        }
    }
}

void FunctionInline::inline_func() {
    for(auto &call : call_list) 
    {
        call_inst_map.clear();
        LOG_DEBUG << "Inline function: " + call.second->get_name();
        LOG_DEBUG << "Instruction: " + call.first->print();
        auto instr = call.first;
        auto func = call.second;
        auto bb = instr->get_parent();
        LOG_DEBUG << bb->get_parent()->get_name();
        LOG_DEBUG <<"bbbbbbbbbbbbbbbbbbbbbbbbbbssdddsd\n" << bb->print();
        int idx = 1;
        for(auto &args:func->get_args())
        {
            call_inst_map[&args] = instr->get_operand(idx++);
        }
        //LOG_INFO <<"module_    ";
        //LOG_DEBUG<< module_->print();
        auto current_inst = instr;
        Instruction *terminator = bb->get_terminator();
        bb->get_instructions().remove(bb->get_terminator());
        for(auto &bb1 : func->get_basic_blocks())
        {
            new_inst_list.clear();
            for(auto &inst : bb1.get_instructions())
            {
                if(inst.is_add())
                {
                    auto new_add = IBinaryInst::create_add(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_add);
                    current_inst = new_add;
                }
                else if(inst.is_sub())
                {
                    auto new_sub = IBinaryInst::create_sub(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_sub);
                    current_inst = new_sub;
                }
                else if(inst.is_mul())
                {
                    auto new_mul = IBinaryInst::create_mul(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_mul);
                    current_inst = new_mul;
                }
                else if(inst.is_div())
                {
                    auto new_div = IBinaryInst::create_sdiv(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_div);
                    current_inst = new_div;
                }
                else if(inst.is_rem())
                {
                    auto new_rem = IBinaryInst::create_srem(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_rem);
                    current_inst = new_rem;
                }
                else if(inst.is_fadd())
                {
                    auto new_fadd = FBinaryInst::create_fadd(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_fadd);
                    current_inst = new_fadd;
                }
                else if(inst.is_fdiv())
                {
                    auto new_fdiv = FBinaryInst::create_fdiv(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_fdiv);
                    current_inst = new_fdiv;
                }
                else if(inst.is_fmul())
                {
                    auto new_fmul = FBinaryInst::create_fmul(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_fmul);
                    current_inst = new_fmul;
                }
                else if(inst.is_fsub())
                {
                    auto new_fsub = FBinaryInst::create_fsub(inst.get_operand(0),inst.get_operand(1),bb);
                    bb->insert_instruction(current_inst,new_fsub);
                    current_inst = new_fsub;
                }
                else if(inst.is_cmp())
                {
                    if(inst.get_instr_op_name() == "sge")
                    {
                        auto new_sge = ICmpInst::create_ge(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_sge);
                        current_inst = new_sge;
                    }
                    else if(inst.get_instr_op_name() == "sle")
                    {
                        auto new_sle = ICmpInst::create_le(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_sle);
                        current_inst = new_sle;
                    }
                    else if(inst.get_instr_op_name() == "sgt")
                    {
                        auto new_sgt = ICmpInst::create_gt(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_sgt);
                        current_inst = new_sgt;
                    }
                    else if(inst.get_instr_op_name() == "slt")
                    {
                        auto new_slt = ICmpInst::create_lt(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_slt);
                        current_inst = new_slt;
                    }
                    else if(inst.get_instr_op_name() == "eq")
                    {
                        auto new_eq = ICmpInst::create_eq(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_eq);
                        current_inst = new_eq;
                    }
                    else if(inst.get_instr_op_name() == "ne")
                    {
                        auto new_ne = ICmpInst::create_ne(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_ne);
                        current_inst = new_ne;
                    }
                }
                else if(inst.is_fcmp())
                {
                    if(inst.get_instr_op_name() == "fge")
                    {
                        auto new_fge = FCmpInst::create_fge(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_fge);
                        current_inst = new_fge;
                    }
                    else if(inst.get_instr_op_name() == "fle")
                    {
                        auto new_fle = FCmpInst::create_fle(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_fle);
                        current_inst = new_fle;
                    }
                    else if(inst.get_instr_op_name() == "fgt")
                    {
                        auto new_fgt = FCmpInst::create_fgt(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_fgt);
                        current_inst = new_fgt;
                    }
                    else if(inst.get_instr_op_name() == "flt")
                    {
                        auto new_flt = FCmpInst::create_flt(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_flt);
                        current_inst = new_flt;
                    }
                    else if(inst.get_instr_op_name() == "feq")
                    {
                        auto new_feq = FCmpInst::create_feq(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_feq);
                        current_inst = new_feq;
                    }
                    else if(inst.get_instr_op_name() == "fne")
                    {
                        auto new_fne = FCmpInst::create_fne(inst.get_operand(0),inst.get_operand(1),bb);
                        bb->insert_instruction(current_inst,new_fne);
                        current_inst = new_fne;
                    }
                }
                else if(inst.is_call())
                {
                    std::vector<Value *> args;
                    for(auto i = 1; i < inst.get_num_operand(); i++)
                    {
                        args.push_back(inst.get_operand(i));
                    }
                    auto func = dynamic_cast<Function *>(inst.get_operand(0));
                    auto new_call = CallInst::create_call(func,args,bb);
                    bb->insert_instruction(current_inst,new_call);
                    current_inst = new_call;
                }
                else if(inst.is_br())
                {
                    auto br = dynamic_cast<BranchInst *>(&inst);
                    if(br->is_cond_br())
                    {
                        auto true_bb = dynamic_cast<BasicBlock *>(inst.get_operand(1));
                        auto false_bb = dynamic_cast<BasicBlock *>(inst.get_operand(2));
                        auto new_br = BranchInst::create_cond_br(inst.get_operand(0),true_bb,false_bb,bb);
                        bb->insert_instruction(current_inst,new_br);
                        current_inst = new_br;
                    }
                    else
                    {
                        auto true_bb = dynamic_cast<BasicBlock *>(inst.get_operand(0));
                        auto new_br = BranchInst::create_br(true_bb,bb);
                        bb->insert_instruction(current_inst,new_br);
                        current_inst = new_br;
                    }
                }
                else if(inst.is_ret())
                {
                    if(inst.get_num_operand() != 0)
                    {
                        auto val = inst.get_operand(0);
                        if(!dynamic_cast<Constant*>(val))
                            ret_val = call_inst_map[inst.get_operand(0)];
                        else
                            ret_val = dynamic_cast<Constant*>(val);
                    }
                }
                else if(inst.is_gep())
                {
                    std::vector<Value *> idx;
                    for(auto i = 1; i < inst.get_num_operand(); i++)
                    {
                        idx.push_back(inst.get_operand(i));
                    }
                    auto new_gep = GetElementPtrInst::create_gep(inst.get_operand(0),idx,bb);
                    bb->insert_instruction(current_inst,new_gep);
                    current_inst = new_gep;
                }
                else if(inst.is_store())
                {
                    auto store = dynamic_cast<StoreInst *>(&inst);
                    auto lval = store->get_lval();
                    auto rval = store->get_rval();
                    auto new_store = StoreInst::create_store(rval,lval,bb);
                    bb->insert_instruction(current_inst,new_store);
                    current_inst = new_store;
                }
                else if(inst.is_load())
                {
                    auto load = dynamic_cast<LoadInst *>(&inst);
                    auto lval = load->get_lval();
                    auto new_load = LoadInst::create_load(lval,bb);
                    bb->insert_instruction(current_inst,new_load);
                    current_inst = new_load;
                }
                else if(inst.is_alloca())
                {
                    auto alloca = dynamic_cast<AllocaInst *>(&inst);
                    auto new_alloca = AllocaInst::create_alloca(alloca->get_type(),bb);
                    bb->insert_instruction(current_inst,new_alloca);
                    current_inst = new_alloca;
                }
                else if(inst.is_zext())
                {
                    auto zext = dynamic_cast<ZextInst *>(&inst);
                    auto new_zext = ZextInst::create_zext(zext->get_operand(0),zext->get_dest_type(),bb);
                    bb->insert_instruction(current_inst,new_zext);
                    current_inst = new_zext;
                }
                else if(inst.is_fp2si())
                {
                    auto fp2si = dynamic_cast<FpToSiInst *>(&inst);
                    auto new_fp2si = FpToSiInst::create_fptosi(fp2si->get_operand(0),fp2si->get_dest_type(),bb);
                    bb->insert_instruction(current_inst,new_fp2si);
                }
                else if(inst.is_si2fp())
                {
                    auto si2fp = dynamic_cast<SiToFpInst *>(&inst);
                    auto new_si2fp = SiToFpInst::create_sitofp(si2fp->get_operand(0),bb);
                    bb->insert_instruction(current_inst,new_si2fp);
                    current_inst = new_si2fp;
                }
                else if(inst.is_phi())
                {
                    auto phi = dynamic_cast<PhiInst *>(&inst);
                    std::vector<Value *> vals;
                    std::vector<BasicBlock *> val_bbs;
                    for(auto i = 0; i < phi->get_num_operand(); i+=2)
                    {
                        vals.push_back(phi->get_operand(i));
                        val_bbs.push_back(dynamic_cast<BasicBlock *>(phi->get_operand(i+1)));
                    }
                    auto new_phi = PhiInst::create_phi(phi->get_type(),bb,vals,val_bbs);
                    bb->insert_instruction(current_inst,new_phi);
                    current_inst = new_phi;
                }
                if(!inst.is_ret())
                {
                    call_inst_map[&inst] = current_inst;
                    new_inst_list.push_back(current_inst);
                }
            }
        }
        bb->add_instruction(terminator);
        LOG_DEBUG << func->get_name();
        //LOG_DEBUG <<"bbbbbbbbbbbbbbbbbbbbbbbbbb\n" << "new_inst_list.size() = " << new_inst_list.size() << "\n";
        for(auto &inst:new_inst_list)
        {
           // LOG_DEBUG << inst->print();
            for(int i = 0;i < inst->get_num_operand();i++)
            {
                if(call_inst_map.find(inst->get_operand(i)) != call_inst_map.end())
                {
                    inst->set_operand(i,call_inst_map[inst->get_operand(i)]);
                }
            }
        }
      // LOG_DEBUG <<"bbbbbbbbbbbbbbbbbbbbbbbbbbssdddsd\n" << bb->print();
        if(ret_val != nullptr)
            instr->replace_all_use_with(ret_val);
        
       // LOG_DEBUG <<"bbbbbbbbbbbbbbbbbbbbbbbbbb\n" << instr->print() << "\n" << instr->get_parent()->get_name();
        bb->get_instructions().remove(instr);
        LOG_DEBUG <<"bbbbbbbbbbbbbbbbbbbbbbbddddddddsssssssbbb\n" << bb->print();

       // bb->erase_instr(instr);
        //LOG_DEBUG <<"bbbbbbbbbbbbbbbbbbbbbbbbbb\n" << bb->print();
    }
}