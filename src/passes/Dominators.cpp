#include "Dominators.hpp"
#include "BasicBlock.hpp"
#include "Type.hpp"
#include "logging.hpp"
#include "ConstProp.hpp"
#include <memory>

void Dominators::run() {
    std::shared_ptr<ConstProp> const_prop = std::make_shared<ConstProp>(m_);
    //printf("Dominators::run\n");
    for (auto &f1 : m_->get_functions()) {
        auto f = &f1;
        if (f->get_basic_blocks().size() == 0)
            continue;
       // LOG_DEBUG << "Dominators: run " << f->get_name();
       //  LOG_DEBUG << f->get_name() << " " << f->get_basic_blocks().size();
        for(auto &bb1 : f->get_basic_blocks())
        {
            const_prop->set_dead_block(&bb1);
        }
        const_prop->find_dead_block(f,f->get_entry_block());
        const_prop->clear_dead_block(f);
        const_prop->clear_dead_block();
       // LOG_DEBUG << f->print();
      //  LOG_DEBUG << f->get_name() << " " << f->get_basic_blocks().size();
        for (auto &bb1 : f->get_basic_blocks()) {
            auto bb = &bb1;
            idom_.insert({bb, {}});
            dom_frontier_.insert({bb, {}});
            dom_tree_succ_blocks_.insert({bb, {}});
        }
        
      //  LOG_DEBUG << "Dominators: create_reverse_post_order" ;
        create_reverse_post_order(f);
       // LOG_DEBUG << "Dominators: create_idom" ;
        create_idom(f);
      //  for(auto &bb1 : f->get_basic_blocks())
       // {
     //       auto bb = &bb1;
     //       LOG_DEBUG << "Dominators: create_idom " << bb->get_name() << " " << get_idom(bb)->get_name();
     //  }
       // LOG_DEBUG << "Dominators: create_dominance_frontier" ;
        create_dominance_frontier(f);
        //LOG_DEBUG << "Dominators: create_dom_tree_succ" ;
        create_dom_tree_succ(f);
    }
}

void Dominators::create_idom(Function *f) {
    // TODO 分析得到 f 中各个基本块的 idom
    for(auto &bb1: f->get_basic_blocks()){
        auto bb = &bb1;
        idom_[bb] = nullptr;
    }
    idom_[f->get_entry_block()] = f->get_entry_block();
    bool changed = true;
    while(changed){
        changed = false;
        for(auto bb : reverse_post_order_){
            if(bb == f->get_entry_block()){
                continue;
            }
            BasicBlock *pre = nullptr;
            for(auto pre_bb : bb->get_pre_basic_blocks()){
                if(get_idom(pre_bb)){
                    pre = pre_bb;
                    break;
                }
            }
              
            BasicBlock *new_idom = pre;
            for(auto pre_bb : bb->get_pre_basic_blocks()){
                if(pre_bb == pre){
                    continue;
                }
                if(get_idom(pre_bb)){
                    new_idom = intersect(pre_bb,new_idom);
                }
            }
            
            if(get_idom(bb) != new_idom){
                idom_[bb] = new_idom;
                changed = true;
            }
        } 
    }
}

void Dominators::create_reverse_post_order(Function *f){
    std::set<BasicBlock *> visited;
    reverse_post_order_.clear();
    post_order_id_.clear();
    post_order_visit(f->get_entry_block(),visited);
    reverse_post_order_.reverse();
}

void Dominators::post_order_visit(BasicBlock *bb,std::set<BasicBlock *> &visited){
    visited.insert(bb);
    for(auto succ_bb : bb->get_succ_basic_blocks()){
        if(visited.find(succ_bb) == visited.end()){
            post_order_visit(succ_bb,visited);
        }
    }
    post_order_id_[bb] = reverse_post_order_.size();
    reverse_post_order_.push_back(bb);
}

BasicBlock *Dominators::intersect(BasicBlock *bb1,BasicBlock *bb2){
    auto finger1 = bb1;
    auto finger2 = bb2;
    while(finger1 != finger2){
        while(post_order_id_[finger1] < post_order_id_[finger2]){
            finger1 = get_idom(finger1);
        }
        while(post_order_id_[finger2] < post_order_id_[finger1]){
            finger2 = get_idom(finger2);
        }
    }
    return finger1;
}

void Dominators::create_dominance_frontier(Function *f) {
    // TODO 分析得到 f 中各个基本块的支配边界集合
    for(auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        if(bb->get_pre_basic_blocks().size() >= 2)
        {
            for(auto pre_bb1 : bb->get_pre_basic_blocks())
            {
                auto runner = pre_bb1;
                while(runner != get_idom(bb))
                {
                  //  LOG_DEBUG << "Dominators: create_dominance_frontier " << runner->get_name() << " " << bb->get_name();
                    dom_frontier_[runner].insert(bb);
                    runner = get_idom(runner);
                }
            }
        }
    }
}

void Dominators::create_dom_tree_succ(Function *f) {
    // TODO 分析得到 f 中各个基本块的支配树后继
    for(auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        auto idom = get_idom(bb);
        ////printf("dom %s\n",idom->get_name().c_str());
        if(idom != bb)
        {
            dom_tree_succ_blocks_[idom].insert(bb);
        }
    }
}


