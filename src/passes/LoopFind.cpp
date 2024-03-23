#include "LoopFind.hpp"
#include "logging.hpp"
#include <memory>

void LoopFind::run() {
    for (auto &func : module_->get_functions()) {
        if(func.is_declaration())
            continue;
        LOG_DEBUG << "loop find Function: " << func.get_name();
        loop_find(&func);
        LOG_DEBUG << "loop find Function: " << func.get_name() << " done";
        for(auto &loop : loops)
        {
            LOG_DEBUG << "Loop: ";
            for(auto &bb: *loop)
            {
                LOG_DEBUG << bb->get_name();
            }
        }
    }
}

void LoopFind::loop_find(Function *func) {
    BB_DFN.clear();
    BB_LOW.clear();
    color.clear();
    cnt = 0;
    tarjan(func->get_entry_block());
    while(!loop_stack.empty()){
        LOG_DEBUG << "find find loop entry " << loop_stack.size() << " loops left";
        auto loop = loop_stack.top();
        loop_stack.pop();
        LOG_DEBUG << loop->size() <<"   "<< loops.at(0)->size() << " loop size";
        LOG_DEBUG << get_loop_entry(loop)->get_name() << " entry";
        if(bb_loop.find(get_loop_entry(loop)) != bb_loop.end())
        {
            parent_loop[loop] = bb_loop[get_loop_entry(loop)];
        }
        for(auto &bb: *loop)
        {
            bb->loop_depth_add(1);
            bb_loop[bb] = loop;
        }
        color[get_loop_entry(loop)] = 3;
        cnt = 0;
        BB_DFN.clear();
        BB_LOW.clear();
        for(auto &succ: get_loop_entry(loop)->get_succ_basic_blocks())
        {
            if(bb_loop[succ] == loop && color[succ] != 3)
            {
                tarjan(succ);
            }
        }
    }

}

void LoopFind::tarjan(BasicBlock *bb)
{
    LOG_DEBUG << "tarjan: " << bb->get_name() << "   " << cnt;
    BB_DFN[bb] = BB_LOW[bb] = ++cnt;
    BB_Stack.push(bb);
    color[bb] = 1;
    for(auto &succ: bb->get_succ_basic_blocks())
    {
        if(color[succ] != 3)
        {
            if(BB_DFN[succ] == 0)
            {
                tarjan(succ);
                BB_LOW[bb] = std::min(BB_LOW[bb],BB_LOW[succ]);
            }
            else if(color[succ] == 1)
            {
                BB_LOW[bb] = std::min(BB_LOW[bb],BB_DFN[succ]);
            }
        }
    }

    if(BB_LOW[bb] == BB_DFN[bb])
    {
        LOG_DEBUG << "Loop in : ";
        Loop* loop = new Loop;
        auto top = BB_Stack.top();
        while(top != bb)
        {
            LOG_DEBUG << top->get_name() << " pop";
            BB_Stack.pop();
            color[top] = 2;
            loop->push_back(top);
            top = BB_Stack.top();
        }
        BB_Stack.pop();
        LOG_DEBUG << top->get_name() <<"   " << bb->get_name() << " pop";
        color[bb] = 2;
        loop->push_back(bb);
        if(loop->size() > 1)
        {
            LOG_DEBUG << "find loop";
            loops.push_back(loop);
            LOG_DEBUG << "push loop " << loop->size();
            loop_stack.push(loop);
            LOG_DEBUG << loop_stack.top()->size();
        }
    }
}