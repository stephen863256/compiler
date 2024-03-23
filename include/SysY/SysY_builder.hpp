#pragma once

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"
#include "ast.hpp"

#include <map>
#include <memory>
#include <vector>

class Scope {
  public:
    // enter a new scope
    void enter() { 
      Variableinner.emplace_back(); 
      Functioninner.emplace_back();
      array_param.emplace_back();
      Constinner.emplace_back();
      }

    // exit a scope
    void exit() { 
      Variableinner.pop_back(); 
      Functioninner.pop_back();
      array_param.pop_back();
      Constinner.pop_back();
    }

    bool in_global() { return Variableinner.size() == 1; }

    // push a name to scope
    // return true if successful
    // return false if this name already exits
    bool push(const std::string& name, Value *val) {
        auto result = Variableinner[Variableinner.size() - 1].insert({name, val});
        return result.second;
    }

    bool push_func(const std::string& name, Value *val) {
        auto result = Functioninner[Functioninner.size() - 1].insert({name, val});
        return result.second;
    }

    bool push_params(std::string name,  std::vector<int> params) {
      auto result = array_param[array_param.size() - 1].insert({name, params});
      return result.second;
    }

    bool push_const(const std::string& name, ConstantArray *val) {
        auto result = Constinner[Constinner.size() - 1].insert({name, val});
        return result.second;
    }
    Value *find(const std::string& name) {
    for (auto s = Variableinner.rbegin(); s != Variableinner.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }

    // Name not found: handled here?
    assert(false && "Name not found in scope");

    return nullptr;
    }

    Value *find_func(const std::string& name) {
    for(auto s = Functioninner.rbegin(); s != Functioninner.rend(); s++) {
      auto iter = s->find(name);
      if (iter != s->end()) {
        return iter->second;
      }
    }
    assert(false && "Name not found in scope");
    
    return nullptr;
    }

    ConstantArray *find_const(const std::string& name) {
    for(auto s = Constinner.rbegin(); s != Constinner.rend(); s++) {
      auto iter = s->find(name);
      if (iter != s->end()) {
        return iter->second;
      }
    }
    //assert(false && "Name not found in scope");
    
    return nullptr;
    }

    std::vector<int> find_params(std::string name) {
    for (auto s = array_param.rbegin(); s != array_param.rend(); s++) {
      auto iter = s->find(name);
      if (iter != s->end()) {
        return iter->second;
      }
    }
    return {};
  }
  private:
    std::vector<std::map<std::string, Value *>> Variableinner;
    std::vector<std::map<std::string, Value *>> Functioninner;
    std::vector<std::map<std::string, ConstantArray *>> Constinner;
    std::vector<std::map<std::string, std::vector<int>>> array_param;
};

class SysYBuilder : public ASTVisitor {
  public:
    SysYBuilder() {
        module = std::make_unique<Module>();
        builder = std::make_unique<IRBuilder>(nullptr, module.get());
        auto *TyVoid = module->get_void_type();
        auto *TyInt32 = module->get_int32_type();
        auto *TyFloat = module->get_float_type();
        auto *TyInt32ptr = module->get_int32_ptr_type();
        auto *TyFloatptr = module->get_float_ptr_type();

        auto *getint_type = FunctionType::get(TyInt32, {});
        auto *getint_fun = Function::create(getint_type, "getint", module.get());

        auto *getch_type = FunctionType::get(TyInt32, {});
        auto *getch_fun = Function::create(getch_type, "getch", module.get());

        auto *getfloat_type = FunctionType::get(TyFloat, {});
        auto *getfloat_fun = Function::create(getfloat_type, "getfloat", module.get());

        std::vector<Type *> putint_params;
        putint_params.push_back(TyInt32);
        auto *putint_type = FunctionType::get(TyVoid, putint_params);
        auto *putint_fun = Function::create(putint_type, "putint", module.get());

        std::vector<Type *> putchar_params;
        putchar_params.push_back(TyInt32);
        auto *putchar_type = FunctionType::get(TyVoid, putchar_params);
        auto *putchar_fun = Function::create(putchar_type, "putch", module.get());

        std::vector<Type *> putfloat_params;
        putfloat_params.push_back(TyFloat);
        auto *putfloat_type = FunctionType::get(TyVoid, putfloat_params);
        auto *putfloat_fun = Function::create(putfloat_type, "putfloat", module.get());

        std::vector<Type *> getarray_params;
        getarray_params.push_back(TyInt32ptr);
        auto *getarray_type = FunctionType::get(TyInt32, getarray_params);
        auto *getarray_fun = Function::create(getarray_type, "getarray", module.get());

        std::vector<Type *> getfarray_params;
        getfarray_params.push_back(TyFloatptr);
        auto *getfarray_type = FunctionType::get(TyInt32, getfarray_params);
        auto *getfarray_fun = Function::create(getfarray_type, "getfarray", module.get());

        std::vector<Type *> putarray_params;
        putarray_params.push_back(TyInt32);
        putarray_params.push_back(TyInt32ptr);
        auto *putarray_type = FunctionType::get(TyVoid, putarray_params);
        auto *putarray_fun = Function::create(putarray_type, "putarray", module.get());

        std::vector<Type *> putfarray_params;
        putfarray_params.push_back(TyInt32);
        putfarray_params.push_back(TyFloatptr);
        auto *putfarray_type = FunctionType::get(TyVoid, putfarray_params);
        auto *putfarray_fun = Function::create(putfarray_type, "putfarray", module.get());

        std::vector<Type *> _sysy_starttime_params;
        _sysy_starttime_params.push_back(TyInt32);
        auto *_sysy_starttime_type = FunctionType::get(TyVoid, _sysy_starttime_params);
        auto *_sysy_starttime_fun = Function::create(_sysy_starttime_type, "_sysy_starttime", module.get());

        std::vector<Type *> _sysy_stoptime_params;
        _sysy_stoptime_params.push_back(TyInt32);
        auto *_sysy_stoptime_type = FunctionType::get(TyVoid, _sysy_stoptime_params);
        auto *_sysy_stoptime_fun = Function::create(_sysy_stoptime_type, "_sysy_stoptime", module.get());
    
        scope.enter();
        scope.push_func("getint", getint_fun);
        scope.push_func("getch", getch_fun);
        scope.push_func("getfloat", getfloat_fun);
        scope.push_func("putint", putint_fun);
        scope.push_func("putch", putchar_fun);
        scope.push_func("putfloat", putfloat_fun);
        scope.push_func("getarray", getarray_fun);
        scope.push_func("getfarray", getfarray_fun);
        scope.push_func("putarray", putarray_fun);
        scope.push_func("putfarray", putfarray_fun);
        scope.push_func("starttime", _sysy_starttime_fun);
        scope.push_func("stoptime", _sysy_stoptime_fun);
    }

    std::unique_ptr<Module> getModule() { return std::move(module); }

  private:
     //virtual Value *visit(ASTNode &) override final;
        virtual Value *visit(ASTCompUnit &) override final;
        virtual Value *visit(ASTDecl &) override final;
        virtual Value *visit(ASTConstDecl &) override final;
       // virtual Value *visit(ASTConstDefList &) override final;
        virtual Value *visit(ASTConstDef &) override final;
        virtual Value *visit(ASTConstInitVal &) override final;
        virtual Value *visit(ASTVarDecl &) override final;
        virtual Value *visit(ASTVarDef &) override final;
       // virtual Value *visit(ASTVarDefList &) override final;
        virtual Value *visit(ASTInitVal &) override final;
        virtual Value *visit(ASTFuncDef &) override final;
        //virtual Value *visit(ASTFuncFParams &) override final;
        virtual Value *visit(ASTFuncFParam &) override final;
        virtual Value *visit(ASTBlock &) override final;
        virtual Value *visit(ASTBlockItem &) override final;
        virtual Value *visit(ASTStmt &) override final;
        virtual Value *visit(ASTExp &) override final;
        virtual Value *visit(ASTCond &) override final;
        virtual Value *visit(ASTLVal &) override final;
        virtual Value *visit(ASTPrimaryExp &) override final;
        virtual Value *visit(ASTNumber &) override final;
        virtual Value *visit(ASTUnaryExp &) override final;
        virtual Value *visit(ASTMulExp &) override final;
        virtual Value *visit(ASTAddExp &) override final;
        virtual Value *visit(ASTRelExp &) override final;
        virtual Value *visit(ASTEqExp &) override final;
        virtual Value *visit(ASTLAndExp &) override final;
        virtual Value *visit(ASTLOrExp &) override final;
        virtual Value *visit(ASTConstExp &) override final;
     //   virtual Value *visit(ASTFuncType &) override final;
     //   virtual Value *visit(ASTConstArrayIdent &) override final;
      //  virtual Value *visit(ASTInitValArrayList &) override final;
      //  virtual Value *visit(ASTFuncArrayIdent &) override final;
      //  virtual Value *visit(ASTArrayIdent &) override final;
        virtual Value *visit(ASTAssignStmt &) override final;
        virtual Value *visit(ASTSelectionStmt &) override final;
        virtual Value *visit(ASTIterationStmt &) override final;
        virtual Value *visit(ASTReturnStmt &) override final;
        virtual Value *visit(ASTBreakStmt &) override final;
        virtual Value *visit(ASTContinueStmt &) override final;
        virtual Value *visit(ASTCall &) override final;

    std::unique_ptr<IRBuilder> builder;
    Scope scope;
    std::unique_ptr<Module> module;

    struct {
        // function that is being built
        Function *func = nullptr;
        // TODO: you should add more fields to store state
       // Value *ret_val = nullptr;
        Value *cur_val = nullptr;
        float cur_const = 0; 
       } context;
};

