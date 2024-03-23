#include "GVN.hpp"

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "DeadCode.hpp"
#include "FuncInfo.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "User.hpp"
#include "logging.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

using namespace GVNExpression;
using std::string_literals::operator""s;
using std::shared_ptr;
/*#define CONST_INT(val1,val2,op) (ConstantInt::get(get_const_int_val(val1) op get_const_int_val(val2), module_))
#define CONST_FP_TO_INT(val1,val2,op) (ConstantInt::get(get_const_float_val(val1) op get_const_float_val(val2), module_))
#define CONST_FLOAT(val1,val2,op) (ConstantFP::get(get_const_float_val(val1) op get_const_float_val(val2), module_))
int get_const_int_val(Value *val) {
    if (auto *const_val = dynamic_cast<ConstantInt *>(val)) {
        return const_val->get_value();
    }
    return -1;
}

float get_const_float_val(Value *val) {
    if (auto *const_val = dynamic_cast<ConstantFP *>(val)) {
        return const_val->get_value();
    }
    return -1;
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value1, Constant *value2)
{
    switch (instr->get_instr_type()) {
        case Instruction::add:  return CONST_INT(value1, value2, +);
        case Instruction::sub:  return CONST_INT(value1, value2, -);
        case Instruction::mul:  return CONST_INT(value1, value2, *);
        case Instruction::sdiv: return CONST_INT(value1, value2, /);
        case Instruction::fadd: return CONST_FLOAT(value1, value2, +);
        case Instruction::fsub: return CONST_FLOAT(value1, value2, -);
        case Instruction::fmul: return CONST_FLOAT(value1, value2, *);
        case Instruction::fdiv: return CONST_FLOAT(value1, value2, /);

        case Instruction::ge: return CONST_INT(value1, value2, >=);
        case Instruction::gt: return CONST_INT(value1, value2, >);
        case Instruction::le: return CONST_INT(value1, value2, <=);
        case Instruction::lt: return CONST_INT(value1, value2, <);
        case Instruction::eq: return CONST_INT(value1, value2, ==);
        case Instruction::ne: return CONST_INT(value1, value2, !=);
        case Instruction::fge: return CONST_FP_TO_INT(value1, value2, >=);
        case Instruction::fgt: return CONST_FP_TO_INT(value1, value2, >);
        case Instruction::fle: return CONST_FP_TO_INT(value1, value2, <=);
        case Instruction::flt: return CONST_FP_TO_INT(value1, value2, <);
        case Instruction::feq: return CONST_FP_TO_INT(value1, value2, ==);
        default: return nullptr;
    }
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value)
{
    switch (instr->get_instr_type()) {
        case Instruction::sitofp: return ConstantFP::get((float)get_const_int_val(value), module_);
        case Instruction::fptosi: return ConstantInt::get((int)get_const_float_val(value), module_);
        case Instruction::zext: return ConstantInt::get(get_const_int_val(value), module_);
        default: return nullptr;
    }
}*/

namespace utils {
static std::string print_congruence_class(const CongruenceClass &cc) {
    std::stringstream ss;
    if (cc.index_ == 0) {
        ss << "top class\n";
        return ss.str();
    }
    ss << "\nindex: " << cc.index_ << "\nleader: " << cc.leader_->print()
       << "\nvalue phi: " << (cc.value_phi_ ? cc.value_phi_->print() : "nullptr"s)
       << "\nvalue expr: " << (cc.value_expr_ ? cc.value_expr_->print() : "nullptr"s) << "\nmembers: {";
    for (auto &member : cc.members_)
        ss << member->print() << "; ";
    ss << "}\n";
    return ss.str();
}

static std::string dump_cc_json(const CongruenceClass &cc) {
    std::string json;
    json += "[";
    for (auto member : cc.members_) {
        if (auto c = dynamic_cast<Constant *>(member))
            json += member->print() + ", ";
        else
            json += "\"%" + member->get_name() + "\", ";
    }
    json += "]";
    return json;
}

static std::string dump_partition_json(const GVN::partitions &p) {
    std::string json;
    json += "[";
    for (auto cc : p)
        json += dump_cc_json(*cc) + ", ";
    json += "]";
    return json;
}

static std::string dump_bb2partition(const std::map<BasicBlock *, GVN::partitions> &map) {
    std::string json;
    json += "{";
    for (auto [bb, p] : map)
        json += "\"" + bb->get_name() + "\": " + dump_partition_json(p) + ",";
    json += "}";
    return json;
}

// logging utility for you
static void print_partitions(const GVN::partitions &p) {
    if (p.empty()) {
        LOG_DEBUG << "empty partitions\n";
        return;
    }
    std::string log;
    for (auto &cc : p)
        log += print_congruence_class(*cc);
    LOG_DEBUG << log; // please don't use std::cout
}
} // namespace utils

void GVN::initPerFunction() {
    next_value_number_ = 1;
    pin_.clear();
    pout_.clear();
}

void GVN::run() {
    std::ofstream gvn_json;
    if (dump_json_) {
        gvn_json.open("gvn.json", std::ios::out);
        gvn_json << "[";
    }

    folder_ = std::make_unique<ConstFolder>(m_);
   // func_info_ = std::make_unique<FuncInfo>(m_);
 //   func_info_->run();
 //   dce_ = std::make_unique<DeadCode>(m_);
  //  dce_->run(); // let dce take care of some dead phis with undef
    for(auto &f: m_->get_functions())
    {
        if(f.get_basic_blocks().empty())
        {
            continue;
        }
        for(auto &bb: f.get_basic_blocks())
        {
            
        }
    }
    for (auto &f : m_->get_functions()) {
        if (f.get_basic_blocks().empty())
            continue;
        func_ = &f;
        initPerFunction();
        LOG_INFO << "Processing " << f.get_name();
        detectEquivalences();
        LOG_INFO << "===============pin=========================\n";
        for (auto &[bb, part] : pin_) {
            LOG_INFO << "\n===============bb: " << bb->get_name() << "=========================\npartitionIn: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        LOG_INFO << "\n===============pout=========================\n";
        for (auto &[bb, part] : pout_) {
            LOG_INFO << "\n=====bb: " << bb->get_name() << "=====\npartitionOut: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        if (dump_json_) {
            gvn_json << "{\n\"function\": ";
            gvn_json << "\"" << f.get_name() << "\", ";
            gvn_json << "\n\"pout\": " << utils::dump_bb2partition(pout_);
            gvn_json << "},";
        }
        replace_cc_members(); // don't delete instructions, just replace them
    }
    dce_->run(); // let dce do that for us
    if (dump_json_)
        gvn_json << "]";
}

bool GVNExpression::operator==(const Expression &lhs, const Expression &rhs) {
    if (lhs.get_expr_type() != rhs.get_expr_type())
        return false;
    if (lhs.get_expr_type() == Expression::e_constant) {
        auto *l = dynamic_cast<const ConstantExpression *>(&lhs);
        auto *r = dynamic_cast<const ConstantExpression *>(&rhs);
        return l->equiv(r);
    }
    else if (lhs.get_expr_type() == Expression::e_var) {
        auto *l = dynamic_cast<const VariableExpression *>(&lhs);
        auto *r = dynamic_cast<const VariableExpression *>(&rhs);
        return l->equiv(r);
    } else if (lhs.get_expr_type() == Expression::e_bin) {
        auto *l = dynamic_cast<const BinaryExpression *>(&lhs);
        auto *r = dynamic_cast<const BinaryExpression *>(&rhs);
        return l->equiv(r);
    } else if (lhs.get_expr_type() == Expression::e_phi) {
        auto *l = dynamic_cast<const PhiExpression *>(&lhs);
        auto *r = dynamic_cast<const PhiExpression *>(&rhs);
        return l->equiv(r);
    }
    return false;
}

bool GVNExpression::operator==(const std::shared_ptr<Expression> &lhs,const std::shared_ptr<Expression> &rhs) {
    if(lhs == nullptr and rhs == nullptr) 
        return true;
    else if(lhs == nullptr or rhs == nullptr)
        return false;
    return *lhs == *rhs;
}

void GVN::replace_cc_members()
{
    for (auto &pout : pout_) {
        auto  bb = pout.first;
        auto part = pout.second;
        for (auto &cc : part) {
            if (cc->index_ == 0)
                continue;
            //auto *leader = cc->leader_;
            for(auto &member : cc->members_) {
                bool member_is_phi = dynamic_cast<PhiInst *>(member);
                bool value_phi = cc->value_phi_ != nullptr;
                if (member != cc->leader_ and (value_phi or !member_is_phi)) {
                    // only replace the members if users are in the same block as bb
                    member->replace_use_with_if(cc->leader_,[bb](Use use)
                    {
                        auto *instr = dynamic_cast<Instruction *>(use.val_);
                        auto parent = instr->get_parent();
                        if(instr->is_phi())
                        {
                            auto pred = parent->get_pre_basic_blocks();
                            if(std::find(pred.begin(), pred.end(), parent) != pred.end())
                                return true;
                            else
                                return false;
                        }
                        else
                        {
                            return parent == bb;
                        }
                    } );
                }
            }        
        }
    }
    return;
}

std::shared_ptr<GVNExpression::Expression> GVN::getVN(const partitions &pout,std::shared_ptr<GVNExpression::Expression> ve)
{
    for (auto &cc : pout) {
        if(cc->value_expr_ != nullptr && cc->value_expr_ == ve)
            return ConstantExpression::create(cc->leader_);
    }
    return nullptr;
}

void GVN::copy_stmt(BasicBlock *bb){
    if(!bb->get_succ_basic_blocks().empty())
    {
        for(auto &succ : bb->get_succ_basic_blocks())
        {
          //  LOG_INFO << "bb->get_succ_basic_blocks()=========================\n" << bb->get_succ_basic_blocks().size() << "\n";
            for(auto &inst: succ->get_instructions())
            {
               // LOG_INFO <<"succ->get_instructions()=========================\n" << succ->get_instructions().size() << "\n";
                if(inst.is_phi())
                {
                 //   LOG_INFO << "===============phi=========================\n";
                    Value *op0 = inst.get_operand(0);
                  //  Value *op1 = inst.get_operand(1);
                    Value *op2 = inst.get_operand(2);
                  //  Value *op3 = inst.get_operand(3);
                    if(inst.get_operand(1) == bb)
                    {
                        bool found = false;
                        for(auto cc = pout_[bb].begin(); cc != pout_[bb].end(); cc++)
                        {
                            if((*cc)->members_.find(&inst) != (*cc)->members_.end())
                            {
                                (*cc)->members_.erase(&inst);
                                if((*cc)->members_.empty())
                                {
                                    pout_[bb].erase(cc);
                                }
                            }
                            auto op0_const = dynamic_cast<Constant *>(op0);
                            auto cc_const_expression = dynamic_cast<ConstantExpression*>((*cc)->value_expr_.get());
                            if((*cc)->members_.find(op0) != (*cc)->members_.end() ||(op0_const != nullptr && cc_const_expression != nullptr && op0 == cc_const_expression->c_))
                            {
                                found = true;
                                (*cc)->members_.insert(&inst);
                            }
                        }
                            if(!found)
                            {
                              //  LOG_INFO << "===============new_cc=========================\n" << "value number: " << next_value_number_ << "\n"
                             //   << "leader: " << inst.print() << "\n";
                                auto new_cc = std::make_shared<CongruenceClass>(next_value_number_ ++);
                                new_cc->leader_ = op0;
                                new_cc->members_.insert(&inst);
                                new_cc->value_expr_ = ConstantExpression::create(op0);
                                pout_[bb].insert(new_cc);
                            }
                        
                    }
                    else if(inst.get_operand(3) == bb)
                    {
                        bool found = false;
                        for(auto cc = pout_[bb].begin(); cc != pout_[bb].end(); cc++)
                        {
                            if((*cc)->members_.find(&inst) != (*cc)->members_.end())
                            {
                                (*cc)->members_.erase(&inst);
                                if((*cc)->members_.empty())
                                {
                                    pout_[bb].erase(cc);
                                }
                            }
                            auto op2_const = dynamic_cast<Constant *>(op2);
                            auto cc_const_expression = dynamic_cast<ConstantExpression*>((*cc)->value_expr_.get());
                            if((*cc)->members_.find(op2) != (*cc)->members_.end() ||(op2_const != nullptr && cc_const_expression != nullptr && op2 == cc_const_expression->c_))
                            {
                                found = true;
                                (*cc)->members_.insert(&inst);
                            }
                        }
                            if(!found)
                            {
                            //    LOG_INFO << "===============new_cc=========================\n" << "value number: " << next_value_number_ << "\n"
                           //     << "leader: " << inst.print() << "\n";
                                auto new_cc = std::make_shared<CongruenceClass>(next_value_number_ ++);
                                new_cc->leader_ = op2;
                                new_cc->members_.insert(&inst);
                                new_cc->value_expr_ = ConstantExpression::create(op2);
                                pout_[bb].insert(new_cc);
                            }
                        
                    }
                }
            }
        }
    }
}

void GVN::detectEquivalences(){
    bool changed = false;
    partitions top;
    top.emplace(std::make_shared<CongruenceClass>(-1));
   // LOG_INFO <<"===============detectEquivalences=========================\n";
    for(auto &bb : func_->get_basic_blocks())
    {
        if(bb.get_pre_basic_blocks().empty())
        {
           // LOG_INFO << "===============get_args=========================\n";
            for(auto & arg : func_->get_args())
            {
                auto cc = std::make_shared<CongruenceClass>(next_value_number_++);
                cc->leader_ = &arg;
                cc->members_.insert(&arg);
             //   cc->value_expr_ = ConstantExpression::create(&arg);
                pin_[&bb].insert(cc);
            }
            LOG_INFO << "===============get_global_variable=========================\n";
            for(auto &global_var : m_->get_global_variable())
            {
                auto cc = std::make_shared<CongruenceClass>(next_value_number_++);
                cc->leader_ = &global_var;
                cc->members_.insert(&global_var);
                pin_[&bb].insert(cc); 
            }
           // LOG_INFO << next_value_number_ <<"   value number" << "\n";
            pout_[&bb] = transferFunction(&bb, pin_[&bb]);
           // LOG_INFO << next_value_number_ <<"   value number" << "\n";
            copy_stmt(&bb);
        }
        else
        {
      //      LOG_INFO << "===============init_pout=========================\n";
            pout_[&bb] =  top;
        }
    }
    auto current_value_number = next_value_number_;
    do{
        changed = false;
        //LOG_INFO << "value number: " << next_value_number_ << "\n";
        next_value_number_ = current_value_number;
       // LOG_INFO << "value number: " << next_value_number_ << "\n";
        for(auto &bb : func_->get_basic_blocks())
        {
            LOG_INFO << "===============bb:=========================\n"<< bb.get_name() << "\n";
            auto pre = bb.get_pre_basic_blocks();
            if(pre.size() == 2)
            {
                LOG_INFO << "===============join=========================\n";
                pin_[&bb] = join(pout_[pre.front()], pout_[pre.back()]);
            }
            else if(pre.size() == 1)
            {
                LOG_INFO << "===============clone=========================\n";
                pin_[&bb] = clone(pout_[pre.front()]);
            }
            else
            {
                continue;
            }
            partitions old_pout = clone(pout_[&bb]);
            pout_[&bb] = transferFunction(&bb, pin_[&bb]);
            copy_stmt(&bb);
            if(pout_[&bb] != old_pout)
            {
                changed = true;
            }
        }       
    }while(changed);
}

//copy data
GVN::partitions GVN::clone(const partitions &p) {
    partitions new_p;
    for (auto &cc : p) {
        new_p.insert(std::make_shared<CongruenceClass>(*cc));
    }
    return new_p;
}

GVN::partitions GVN::join(const partitions &P1, const partitions &P2)
{
    partitions P = {};
    if((*P1.begin())->index_ == (size_t)(-1))
    {
        LOG_DEBUG << "P1 is top\n";
        return P2;
    }
    if((*P2.begin())->index_ == (size_t)(-1))
    {
         LOG_DEBUG << "P2 is top\n";
        return P1;
    }
    for(auto &cc1 : P1)
    {
        for(auto &cc2 : P2)
        {
            auto Ck = intersect(cc1, cc2);
            if(Ck != nullptr)
            {
                P.insert(Ck);
            }
        }
    }
    return P;
}

std::shared_ptr<CongruenceClass> GVN::intersect(std::shared_ptr<CongruenceClass> Ci, std::shared_ptr<CongruenceClass> Cj)
{
    auto Ck = std::make_shared<CongruenceClass>(0);
    
   /* if(Ci->index_ == Cj->index_)
    {
        Ck->index_ = Ci->index_;
    }
    
   if(Ci->leader_ == Cj->leader_)
    {
        Ck->leader_ = Ci->leader_;
    }*/
    if(*Ci == *Cj)
    {
        Ck->index_ = Ci->index_;
        Ck->leader_ = Ci->leader_;
    }
    
    
    if ((Ci->value_expr_ == Cj->value_expr_) )
//     ||   (Ci->value_expr_ && Cj->value_expr_ && *Ci->value_expr_ == *Cj->value_expr_)) 
    {
        Ck->value_expr_ = Ci->value_expr_;
    }
    if ((Ci->value_phi_ == Cj->value_phi_) 
    ||    (Ci->value_phi_ && Cj->value_phi_ && *Ci->value_phi_ == *Cj->value_phi_)) 
    {
        Ck->value_phi_ = Ci->value_phi_;
    }
    for(auto &member : Ci->members_)
    {
        if(Cj->members_.find(member) != Cj->members_.end())
        {
            Ck->members_.insert(member);
        }
    }
    if(Ck->members_.empty())
    {
        return nullptr;
    }
    else {
        if(Ck->index_ == 0)
        {
            Ck->index_ = next_value_number_++;
        }
        auto vi = ConstantExpression::create(Ci->leader_);
        auto vj = ConstantExpression::create(Cj->leader_);
        if(Ck->leader_ == nullptr)
        {
            Ck->leader_ = *Ck->members_.begin();
        }
        Ck->value_phi_ = PhiExpression::create(vi, vj);
    }
    return Ck;
}

GVN::partitions GVN::transferFunction(BasicBlock *bb, partitions pin)
{
    partitions pout = clone(pin);
   // LOG_INFO << "===============transferFunction=========================\n";
    for(auto &inst : bb->get_instructions())
    {
        if(inst.is_phi() || inst.is_void())
        {
            continue;
        }
        for (auto cc = pout.begin() ; cc != pout.end(); cc++) {
            if ((*cc)->members_.find(&inst) != (*cc)->members_.end()) {
                (*cc)->members_.erase(&inst);
                if ((*cc)->members_.empty()) {
                    cc = pout.erase(cc);
                }
            }
        }
        auto ve = valueExpr(pout, &inst);
        auto vpf = valuePhiFunc(bb,ve,pout);
        auto findcc = false;
        if(ve != nullptr && !inst.is_load())
        {
            for(auto &cc : pout)
            {
                if((cc->value_expr_ == ve) || (vpf && cc->value_phi_ && *cc->value_phi_ == *vpf))
                {
                    cc->members_.insert(&inst);
                    findcc = true;
                    break;
                }
            }
        }
        if(!findcc)
        {
            LOG_INFO << "===============new_cc=========================\n" << "value number: " << next_value_number_ << "\n" 
            <<"leader: " << inst.print() << "\n";
            auto new_cc = std::make_shared<CongruenceClass>(next_value_number_++);
        //    LOG_INFO << "value number: " << next_value_number_ << "\n";
            new_cc->leader_ = &inst;
            if(inst.isBinary() || inst.is_cmp() || inst.is_fcmp())
            {
                auto op0 = inst.get_operand(0);
                auto op1 = inst.get_operand(1);
                auto op0_const = dynamic_cast<Constant *>(op0);
                auto op1_const = dynamic_cast<Constant *>(op1);
                for(auto &cc : pout)
                {
                    if((cc->members_.find(op0) != cc->members_.end()) && !op0_const)
                    {
                        op0_const = dynamic_cast<Constant *>(cc->leader_);    
                    }
                    if((cc->members_.find(op1) != cc->members_.end()) && !op1_const)
                    {
                        op1_const = dynamic_cast<Constant *>(cc->leader_);    
                    }
                }
                if(op0_const && op1_const)
                {
                    new_cc->leader_ = folder_->compute(&inst, op0_const, op1_const);    
                }
            }
            else if(inst.is_fp2si() || inst.is_si2fp() || inst.is_zext())
            {
                auto op0 = inst.get_operand(0);
                auto op0_const = dynamic_cast<Constant *>(op0);
                for(auto &cc : pout)
                {
                    if((cc->members_.find(op0) != cc->members_.end()) && !op0_const)
                    {
                        op0_const = dynamic_cast<Constant *>(cc->leader_);    
                    }
                }
                if(op0_const)
                {
                    new_cc->leader_ = folder_->compute(&inst, op0_const);
                }
            }
            new_cc->members_.insert(&inst);
            new_cc->value_expr_ = ve;
            new_cc->value_phi_ = vpf;
            pout.insert(new_cc);
        }
    }
    return pout;
}

std::shared_ptr<GVNExpression::Expression> GVN::valueExpr(partitions p, Instruction *instr)
{
    if(instr->get_num_operand() > 2 || instr->is_call() || instr->is_fp2si() || instr->is_si2fp() || instr->is_alloca()
    || instr->is_gep() || instr->is_zext())
    {
        int type;
        if(instr->is_gep())
        {
            type = 0;
        }
        else if(instr->is_call())
        {
            if(func_info_->is_pure_function(dynamic_cast<Function *>(instr->get_operand(0))))
            {
                type = 1;
            }
            else
            {
                type = 2;
            }
        }
        else if(instr->is_si2fp())
        {
            type = 3;
        }
        else if(instr->is_fp2si())
        {
            type = 4;
        }
        else if(instr->is_zext())
        {
            type = 5;
        }
        else
        {
            type = 6;
        }
        std::vector<std::shared_ptr<GVNExpression::Expression>> exps;
        for(auto &op : instr->get_operands())
        {
            auto op0 = op;
            for(auto &cc : p)
            {
                if(cc->members_.find(op) != cc->members_.end())
                {
                    op0 = cc->leader_;
                }
            }
            exps.push_back(ConstantExpression::create(op0));
        }
        return VariableExpression::create(exps, type);
    }
    else if(instr->get_num_operand() == 2)
    {
        auto op0 = instr->get_operand(0);
        auto op1 = instr->get_operand(1);
        auto lhs = op0;
        auto rhs = op1;
        for(auto &cc : p)
        {
            if(cc->members_.find(op0) != cc->members_.end())
            {
                lhs = cc->leader_;
            }
            if(cc->members_.find(op1) != cc->members_.end())
            {
                rhs = cc->leader_;
            }
        }
        auto lhs_exp = ConstantExpression::create(lhs);
        auto rhs_exp = ConstantExpression::create(rhs);
        auto exp = BinaryExpression::create(instr->get_instr_type(), lhs_exp, rhs_exp);
       /* if(instr->is_cmp())
        {
            switch(instr->get_instr_type())
            {
                case Instruction::ge:
                    exp->cmp = GVNExpression::BinaryExpression::GE;   
                    break;
                case Instruction::gt:
                    exp->cmp = GVNExpression::BinaryExpression::GT;   
                    break;
                case Instruction::le:
                    exp->cmp = GVNExpression::BinaryExpression::LE;   
                    break;
                case Instruction::lt:
                    exp->cmp = GVNExpression::BinaryExpression::LT;   
                    break;
                case Instruction::eq:
                    exp->cmp = GVNExpression::BinaryExpression::EQ;   
                    break;
                case Instruction::ne:
                    exp->cmp = GVNExpression::BinaryExpression::NE;   
                    break;
                default:
                    break;
            }
        }*/
        return exp;
    }
    else
    {
        return ConstantExpression::create(instr->get_operand(0));
    }
}

std::shared_ptr<GVNExpression::PhiExpression> GVN::valuePhiFunc(BasicBlock *bb, std::shared_ptr<GVNExpression::Expression> ve, partitions p)
{
    std::shared_ptr<GVNExpression::PhiExpression> vpf = nullptr;
    auto ve_bin = dynamic_cast<BinaryExpression *>(ve.get());
    if(ve_bin == nullptr)
    {
        return nullptr;
    }
    else {
        if(ve_bin->op_ == Instruction::eq || ve_bin->op_ == Instruction::ne 
        || ve_bin->op_ == Instruction::ge || ve_bin->op_ == Instruction::gt 
        || ve_bin->op_ == Instruction::le || ve_bin->op_ == Instruction::lt
        || ve_bin->op_ == Instruction::feq || ve_bin->op_ == Instruction::fne 
        || ve_bin->op_ == Instruction::fge || ve_bin->op_ == Instruction::fgt 
        || ve_bin->op_==Instruction::fle || ve_bin->op_==Instruction::flt)
        {
            return nullptr;
        }
        else 
        {
            auto lhs = dynamic_cast<ConstantExpression *>(ve_bin->lhs_.get());
            auto rhs = dynamic_cast<ConstantExpression*>(ve_bin->rhs_.get());
            auto lhs_value = lhs->c_;
            auto rhs_value = rhs->c_;
            std::shared_ptr<PhiExpression> lhs_phi, rhs_phi;
            if(dynamic_cast<Constant *>(lhs_value) || dynamic_cast<Constant *>(rhs_value))
            {
                return nullptr;
            }
            else
            {
                for(auto &cc : p)
                {
                    if(cc->leader_ == lhs_value)
                    {
                        lhs_phi = cc->value_phi_;   
                    }
                    if(cc->leader_ == rhs_value)
                    {
                        rhs_phi = cc->value_phi_;   
                    }
                }    
            }
            auto pre = bb->get_pre_basic_blocks();
            if(pre.size() != 2)
            {
                return nullptr;
            }
            else 
            {
                std::shared_ptr<BinaryExpression> Vi1_op_Vi2, Vj1_op_Vj2; 
                if(lhs_phi == nullptr || rhs_phi == nullptr)
                {
                    return nullptr;
                }
                else
                {
                    Vi1_op_Vi2 = BinaryExpression::create(ve_bin->op_, lhs_phi->lhs_, rhs_phi->rhs_);
                    Vj1_op_Vj2 = BinaryExpression::create(ve_bin->op_, lhs_phi->rhs_, lhs_phi->rhs_);
                }
                auto Vi = getVN(pout_[pre.front()], Vi1_op_Vi2);
                if(Vi == nullptr)
                {
                    Vi = valuePhiFunc(pre.front(), Vi1_op_Vi2, pout_[pre.front()]);
                    if(Vi != nullptr)
                    {
                        for(auto &cc : pout_[pre.front()])
                        {
                            if(*cc->value_phi_ == *Vi)
                            {
                                Vi = ConstantExpression::create(cc->leader_);
                            }
                        }
                    }
                }
                auto Vj = getVN(pout_[pre.back()], Vj1_op_Vj2);
                if(Vj == nullptr)
                {
                    Vj = valuePhiFunc(pre.back(), Vj1_op_Vj2, pout_[pre.back()]);
                    if(Vj != nullptr)
                    {
                        for(auto &cc : pout_[pre.back()])
                        {
                            if(*cc->value_phi_ == *Vj)
                            {
                                Vj = ConstantExpression::create(cc->leader_);
                            }
                        }
                    }
                }
                if(Vi == nullptr || Vj == nullptr)
                {
                    return nullptr;
                }
                else
                {
                    vpf = PhiExpression::create(Vi, Vj);
                }
            }
        }
    }
    return vpf;
}


bool CongruenceClass::operator==(const CongruenceClass &other) const {
    return this->index_ == other.index_;
}

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2)
{
    if(p1.size() != p2.size())
    {
        return false;
    }
    for(auto &cc1 : p1)
    {
        bool found = false;
        for(auto &cc2 : p2)
        {
            if(*cc1 == *cc2)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            return false;
        }
    }
    return true;
}