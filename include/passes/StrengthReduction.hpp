#pragma once

#include "Function.hpp"
#include "PassManager.hpp"
#include "Module.hpp"
#include "logging.hpp"


class StrengthReduction : public Pass{
    public:
        StrengthReduction(Module *m) : Pass(m){module_ = m;};
        void run() override;
        void strength_reduction(Function *f);

    private:
        Module *module_;
};