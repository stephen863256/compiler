#include "BasicBlock.hpp"
#include "Function.hpp"
#include "IRprinter.hpp"
#include "Module.hpp"
#include "logging.hpp"

#include <cassert>

BasicBlock::BasicBlock(Module *m, const std::string &name = "",
                       Function *parent = nullptr)
    : Value(m->get_label_type(), name), parent_(parent) {
    assert(parent && "currently parent should not be nullptr");
    parent_->add_basic_block(this);
}

Module *BasicBlock::get_module() { return get_parent()->get_parent(); }
void BasicBlock::erase_from_parent() { this->get_parent()->remove(this); }

bool BasicBlock::is_terminated() const {
    if (instr_list_.empty())
        return false;
    switch (instr_list_.back().get_instr_type()) {
    case Instruction::ret:
    case Instruction::br:
        return true;
    default:
        return false;
    }
}

Instruction *BasicBlock::get_terminator() {
    assert(is_terminated() &&
           "Trying to get terminator from an bb which is not terminated");
    return &instr_list_.back();
}

void BasicBlock::add_instruction(Instruction *instr) {
    assert(not is_terminated() && "Inserting instruction to terminated bb");
    instr_list_.push_back(instr);
}

void BasicBlock::replace_instruction(Instruction *old_instr,Instruction *new_instr) {
    auto iter = instr_list_.begin();
    while (iter != instr_list_.end()) {
        if (&*iter == old_instr) {
            instr_list_.insert(iter, new_instr);
            instr_list_.erase(iter);
            return;
        }
        iter++;
    }
}
std::string BasicBlock::print() {
    std::string bb_ir;
    //LOG_DEBUG << "print bb: \n\n\n\n" << this->get_name();
    bb_ir += this->get_name();
    bb_ir += ":";
    // print prebb
    if (!this->get_pre_basic_blocks().empty()) {
        bb_ir += "                                                ; preds = ";
    }
   // LOG_DEBUG << "print prebb: " << this->get_pre_basic_blocks().size();
    for (auto bb : this->get_pre_basic_blocks()) {
        if (bb != *this->get_pre_basic_blocks().begin())
            bb_ir += ", ";
        bb_ir += print_as_op(bb, false);
    }

    // print prebb
    if (!this->get_parent()) {
        bb_ir += "\n";
        bb_ir += "; Error: Block without parent!";
    }
    bb_ir += "\n";
  //  LOG_DEBUG << "print instr in block: " << this->get_instructions().size();
     for (auto &instr : this->get_instructions()) {
       // LOG_DEBUG << "print instr in bloc0 00  0k:     " << instr.print();
     }
    for (auto &instr : this->get_instructions()) {
       // LOG_DEBUG << "print instr in bloc0 00  0k: " << instr.print();
        bb_ir += "  ";
      //  LOG_DEBUG << "print instr in block: " << instr.print();
        bb_ir += instr.print();
      //  LOG_DEBUG << "print instr in block: sssjdjd" << instr.print();
        bb_ir += "\n";
    }

    return bb_ir;
}
