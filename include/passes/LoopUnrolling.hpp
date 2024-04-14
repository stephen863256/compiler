#pragma once

#include "PassManager.hpp"
#include "LoopFind.hpp"
#include "Module.hpp"

class LoopUnrolling : public Pass {
    public:
        LoopUnrolling(Module *m) : Pass(m){module_ = m;};
        void run() override;
        void unroll_const_loop(int count,Loop *loop);
        void unroll_loop(Loop *loop);
        int is_unrollable(Loop *loop);

    private:
        Module *module_;
        std::unique_ptr<LoopFind> loop_find;
};
