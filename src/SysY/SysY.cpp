#include "CodeGen.hpp"
#include "DeadCode.hpp"
#include "Mem2Reg.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "SysY_builder.hpp"
#include "ConstProp.hpp"
#include "UnconditionalBr.hpp"
#include"FunctionInline.hpp"
#include "logging.hpp"
#include "LocalCommonSubExpr.hpp"
//#include "GVN.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include "LoopFind.hpp"
#include "LoopInvariant.hpp"
using std::string;
using std::operator""s;

struct Config {
    string exe_name; // compiler exe name
    std::filesystem::path input_file;
    std::filesystem::path output_file;

    bool emitllvm{false};
    bool emitasm{false};
    bool mem2reg{false};
   // bool gvn{false};
   // bool dumpjson{false};

    Config(int argc, char **argv) : argc(argc), argv(argv) {
        parse_cmd_line();
        check();
    }

  private:
    int argc{-1};
    char **argv{nullptr};

    void parse_cmd_line();
    void check();
    // print helper infomation and exit
    void print_help() const;
    void print_err(const string &msg) const;
};

int main(int argc, char **argv) {
    Config config(argc, argv);

    std::unique_ptr<Module> m;
    {
        auto syntax_tree = parse(config.input_file.c_str());
        auto ast = AST(syntax_tree);
        //CminusfBuilder builder;
        SysYBuilder builder;
        ast.run_visitor(builder);
        m = builder.getModule();
    }

    LOG_DEBUG << m->print();
    PassManager PM(m.get());

    if (config.mem2reg) {
        //PM.add_pass<ConstProp>();
        PM.add_pass<Mem2Reg>();
        LOG_DEBUG << "mem2regssjdjddldldld";
        PM.add_pass<DeadCode>();
       // PM.add_pass<LoopFind>();
        PM.add_pass<LocalCommonSubExpr>();
        PM.add_pass<ConstProp>();
        PM.add_pass<FunctionInline>();
        PM.add_pass<DeadCode>();
        PM.add_pass<UnconditionalBr>();
        PM.add_pass<LoopInvariant>();
        //PM.add_pass<LoopFind>();
       // PM.add_pass<LocalCommonSubExpr>();
        PM.add_pass<DeadCode>();
        PM.add_pass<UnconditionalBr>();
        PM.add_pass<ConstProp>();
        PM.add_pass<DeadCode>();
        PM.add_pass<UnconditionalBr>();
    }
   // if(config.gvn){
   //     PM.add_pass<GVN>(config.dumpjson);
   // }
  
    PM.run();
   //  LOG_INFO << m->print();
    std::ofstream output_stream(config.output_file);
    if (config.emitllvm) {
        auto abs_path = std::filesystem::canonical(config.input_file);
        output_stream << "; ModuleID = 'SysY'\n";
        output_stream << "source_filename = " << abs_path << "\n\n";
        output_stream << m->print();
    } else if (config.emitasm) {
        CodeGen codegen(m.get());
        codegen.run();
        output_stream << codegen.print();
    }

    return 0;
}

void Config::parse_cmd_line() {
    exe_name = argv[0];
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == "-h"s || argv[i] == "--help"s) {
            print_help();
        } else if (argv[i] == "-o"s) {
            if (output_file.empty() && i + 1 < argc) {
                output_file = argv[i + 1];
                i += 1;
            } else {
                print_err("bad output file");
            }
        } else if (argv[i] == "-emit-llvm"s) {
            emitllvm = true;
        } else if (argv[i] == "-S"s) {
            emitasm = true;
        } else if (argv[i] == "-mem2reg"s) {
            mem2reg = true;
        }
      //  } else if (argv[i] == "-gvn"s) {
      //      gvn = true;
       // } else if (argv[i] == "-dumpjson"s) {
      //      dumpjson = true;
      //  } 
        else {
            if (input_file.empty()) {
                input_file = argv[i];
            } else {
                string err =
                    "unrecognized command-line option \'"s + argv[i] + "\'"s;
                print_err(err);
            }
        }
    }
}

void Config::check() {
    if (input_file.empty()) {
        print_err("no input file");
    }
    if (input_file.extension() != ".sy") {
        print_err("file format not recognized");
    }
    if (emitllvm and emitasm) {
        print_err("emit llvm and emit asm both set");
    }
    if (not emitllvm and not emitasm) {
        print_err("not supported: generate executable file directly");
    }
    if (output_file.empty()) {
        output_file = input_file.stem();
        if (emitllvm) {
            output_file.replace_extension(".ll");
        } else if (emitasm) {
            output_file.replace_extension(".s");
        }
    }
}

void Config::print_help() const {
    std::cout << "Usage: " << exe_name
              << " [-h|--help] [-o <target-file>] [-mem2reg] [-emit-llvm] [-S] "
                 "<input-file>"
              << std::endl;
    exit(0);
}

void Config::print_err(const string &msg) const {
    std::cout << exe_name << ": " << msg << std::endl;
    exit(-1);
}
