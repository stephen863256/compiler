#pragma once
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "DeadCode.hpp"
#include "FuncInfo.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "Value.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
namespace GVNExpression {

// fold the constant value
/*class ConstFolder {
  public:
    ConstFolder(Module *m) : module_(m) {}
    Constant *compute(Instruction *instr, Constant *value1, Constant *value2);
    Constant *compute(Instruction *instr, Constant *value1);
   
  private:
    Module *module_;
};*/

/**
 * for constructor of class derived from `Expression`, we make it public
 * because `std::make_shared` needs the constructor to be publicly available,
 * but you should call the static factory method `create` instead the constructor itself to get the desired data
 */
class Expression {
  public:
    enum gvn_expr_t { e_constant, e_bin, e_phi, e_var};
    Expression(gvn_expr_t t) : expr_type(t) {}
    virtual ~Expression() = default;
    virtual std::string print() = 0;
    gvn_expr_t get_expr_type() const { return expr_type; }

  private:
    gvn_expr_t expr_type;
};

bool operator==(const std::shared_ptr<Expression> &lhs, const std::shared_ptr<Expression> &rhs);
bool operator==(const GVNExpression::Expression &lhs, const GVNExpression::Expression &rhs);

class ConstantExpression : public Expression {
  public:
    static std::shared_ptr<ConstantExpression> create(Value *c) { return std::make_shared<ConstantExpression>(c); }
    virtual std::string print() { return c_->print(); }
    // we leverage the fact that constants in lightIR have unique addresses
    bool equiv(const ConstantExpression *other) const { return c_ == other->c_; }
    ConstantExpression(Value *c) : Expression(e_constant), c_(c) {}

    Value *c_;
};

// arithmetic expression
class BinaryExpression : public Expression {
  public:
    static std::shared_ptr<BinaryExpression> create(Instruction::OpID op,
                                                    std::shared_ptr<Expression> lhs,
                                                    std::shared_ptr<Expression> rhs) {
        return std::make_shared<BinaryExpression>(op, lhs, rhs);
    }
    virtual std::string print() {
        return "(" + print_op_name(op_) + " " + lhs_->print() + " " + rhs_->print() + ")";
    }

    bool equiv(const BinaryExpression *other) const {
        if (op_ == other->op_ and *lhs_ == *other->lhs_ and *rhs_ == *other->rhs_){
          //  if (cmp == other->cmp)
                return true;
            //return false;
        }
        else
            return false;
    }

    std::string print_op_name(Instruction::OpID id)
    {
        switch (id) {
            case Instruction::ret:
                return "ret";
            case Instruction::br:
                return "br";
            case Instruction::add:
                return "add";
            case Instruction::sub:
                return "sub";
            case Instruction::mul:
                return "mul";
            case Instruction::sdiv:
                return "sdiv";
            case Instruction::srem:
                return "srem";
            case Instruction::fadd:
                return "fadd";
            case Instruction::fsub:
                return "fsub";
            case Instruction::fmul:
                return "fmul";
            case Instruction::fdiv:
                return "fdiv";
            case Instruction::alloca:
                return "alloca";
            case Instruction::load:
                return "load";
            case Instruction::store:
                return "store";
            case Instruction::ge:
                return "sge";
            case Instruction::gt:
                return "sgt";
            case Instruction::le:
                return "sle";
            case Instruction::lt:
                return "slt";
            case Instruction::eq:
                return "eq";
            case Instruction::ne:
                return "ne";
            case Instruction::fge:
                return "uge";
            case Instruction::fgt:
                return "ugt";
            case Instruction::fle:
                return "ule";
            case Instruction::flt:
                return "ult";
            case Instruction::feq:
                return "ueq";
            case Instruction::fne:
                return "une";
            case Instruction::phi:
                return "phi";
            case Instruction::call:
                return "call";
            case Instruction::getelementptr:
                return "getelementptr";
            case Instruction::zext:
                return "zext";
            case Instruction::fptosi:
                return "fptosi";
            case Instruction::sitofp:
                return "sitofp";
        }
    assert(false && "Must be bug");
    }



    BinaryExpression(Instruction::OpID op, std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs)
        : Expression(e_bin), op_(op), lhs_(lhs), rhs_(rhs) {}

    Instruction::OpID op_;
  /*  enum CmpOp {
        EQ, // ==
        NE, // !=
        GT, // >
        GE, // >=
        LT, // <
        LE  // <=
    };*/
  //  CmpOp cmp;
    std::shared_ptr<Expression> lhs_, rhs_;
};

class PhiExpression : public Expression {
  public:
    static std::shared_ptr<PhiExpression> create(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs) {
        return std::make_shared<PhiExpression>(lhs, rhs);
    }
    virtual std::string print() { return "(phi " + lhs_->print() + " " + rhs_->print() + ")"; }
    bool equiv(const PhiExpression *other) const {
        if (*lhs_ == *other->lhs_ and *rhs_ == *other->rhs_)
            return true;
        else
            return false;
    }
    PhiExpression(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs)
        : Expression(e_phi), lhs_(lhs), rhs_(rhs) {}

    std::shared_ptr<Expression> lhs_, rhs_;
};

class VariableExpression : public Expression {
public:
    static std::shared_ptr<VariableExpression> create(std::vector<std::shared_ptr<Expression>> exps, int type) {
        return std::make_shared<VariableExpression>(exps, type);
    }
    virtual std::string print() {
        std::string result = "(var ";
        for (auto &i : exps_) {
            result += i->print() + " ";
        }
        result += ")";
        return result;
    }
    bool equiv(const VariableExpression *other) const {
        if (this == other) {
            return true;
        }
        if (exps_.size() != other->exps_.size()) {
            return false;
        }
        if (type_ != other->type_) {
            return false;
        }
        if (type_ == 2 || type_ == 6) {
            return false;
        }
        bool flag = true;
        for (size_t i = 0; i < exps_.size(); i++) {
            if ((*exps_[i]) == (*other->exps_[i])) {
                flag = true;
            } else {
                flag = false;
                break;
            }
        }
        return flag;
    }
    VariableExpression(std::vector<std::shared_ptr<Expression>> exps, int type) : Expression(e_var), exps_(exps), type_(type) {}
    std::vector<std::shared_ptr<Expression>> exps_;
    int type_; // 0 getelementptr 1 pure func 2 not pure func 3 si2fp 4 fp2si 5 zext 6 alloca
};
} // namespace GVNExpression

/**
 * Congruence class in each partitions
 * note: for constant propagation, you might need to add other fields
 * and for load/store redundancy detection, you most certainly need to modify the class
 */
struct CongruenceClass {
    size_t index_;
    // representative of the congruence class, used to replace all the members (except itself) when analysis is done
    Value *leader_;
    // value expression in congruence class
    std::shared_ptr<GVNExpression::Expression> value_expr_;
    // value φ-function is an annotation of the congruence class
    std::shared_ptr<GVNExpression::PhiExpression> value_phi_;
    // equivalent variables in one congruence class
    std::set<Value *> members_;

    CongruenceClass(size_t index) : index_(index), leader_{}, value_expr_{}, value_phi_{}, members_{} {}

    bool operator<(const CongruenceClass &other) const { return this->index_ < other.index_; }
    bool operator==(const CongruenceClass &other) const;
};

namespace std {
template <>
// overload std::less for std::shared_ptr<CongruenceClass>, i.e. how to sort the congruence classes
struct less<std::shared_ptr<CongruenceClass>> {
    bool operator()(const std::shared_ptr<CongruenceClass> &a, const std::shared_ptr<CongruenceClass> &b) const {
        // nullptrs should never appear in partitions, so we just dereference it
        return *a < *b;
    }
};
} // namespace std

class GVN : public Pass {
  public:
    using partitions = std::set<std::shared_ptr<CongruenceClass>>;
    GVN(Module *m, bool dump_json) : Pass(m), dump_json_(dump_json) {}
    // pass start
    void run() override;
    // init for pass metadata;
    void initPerFunction();

    void detectEquivalences();
    partitions join(const partitions &P1, const partitions &P2);
    std::shared_ptr<CongruenceClass> intersect(std::shared_ptr<CongruenceClass> Ci, std::shared_ptr<CongruenceClass> Cj);
    partitions transferFunction(BasicBlock *bb, partitions pin);
    std::shared_ptr<GVNExpression::PhiExpression> valuePhiFunc(BasicBlock *bb, std::shared_ptr<GVNExpression::Expression> ve, partitions p);
    std::shared_ptr<GVNExpression::Expression> valueExpr(partitions p, Instruction *instr);
    std::shared_ptr<GVNExpression::Expression> getVN(const partitions &pout,
                                                     std::shared_ptr<GVNExpression::Expression> ve);
    void copy_stmt(BasicBlock *bb);
    // replace cc members with leader
    void replace_cc_members();

    // note: be careful when to use copy constructor or clone
    partitions clone(const partitions &p);


    std::shared_ptr<CongruenceClass> createCongruenceClass(size_t index = 0) {
        return std::make_shared<CongruenceClass>(index);
    }

  private:
    bool dump_json_;
    std::uint64_t next_value_number_ = 1;
    Function *func_;
    std::map<BasicBlock *, partitions> pin_, pout_;
    std::unique_ptr<FuncInfo> func_info_;
    std::unique_ptr<DeadCode> dce_;
};

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2);