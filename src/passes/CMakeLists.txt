add_library(
    opt_lib STATIC
    Dominators.cpp
    Mem2Reg.cpp
    FuncInfo.cpp
    DeadCode.cpp
    ConstProp.cpp
    UnconditionalBr.cpp
    FunctionInline.cpp
    LoopFind.cpp
    LoopInvariant.cpp
    LoopUnrolling.cpp
    LocalCommonSubExpr.cpp
    StrengthReduction.cpp
    InstCombine.cpp
)
