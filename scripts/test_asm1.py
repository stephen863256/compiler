import os
import subprocess
import time

#EXE_PATH = os.path.abspath("build/compiler")
LIB_PATH = os.path.abspath("src/io/sylib.c")

TEST_DIRS = [
    "supplement/functional",
    "supplement/performance",
    "supplement/final_performance",
    "supplement/hidden_functional",
]

def evaluate(test_base_path, testcases):
    print("===========TEST START==============")
    print("now in {}".format(test_base_path))     
    wrong_cases = []
    wrong_answers = []
    for index, case in enumerate(testcases):
        print('Case %s:'% case, end='\n')
        test_path = os.path.join(test_base_path, case)
        sy_path = test_path + ".sy"
        asm_path = test_path + ".s"
        input_path = test_path + ".in"
        output_path = test_path + ".out"
        exe_path = "./" + test_path + ".exe"


        need_input = testcases[case]
        
        ASM_GEN_PTN = ["SysY", "-S", sy_path, "-mem2reg" ,"-o", asm_path]
        EXE_GEN_PTN = ["gcc" ,asm_path, LIB_PATH ,"-o", exe_path]
        EXE_PTN = [exe_path]
                    
        asmbuilder_result = subprocess.run(ASM_GEN_PTN, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        if asmbuilder_result.returncode != 0:
            print(asmbuilder_result.stderr, end='')
            print('\t\033[31mCompiler Execute Fail\033[0m')
            continue
            
        if asmbuilder_result.returncode == 0:
            input_option = None
            if need_input:
                with open(input_path, "r") as fin:
                    input_option = fin.read()
                    
            try:
                res = subprocess.run(EXE_GEN_PTN, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
                if res.returncode != 0:
                    print(res.stderr, end='')
                    print('\t\033[31mRV64 GCC Execute Fail\033[0m')
                    continue
                start_time = time.perf_counter()
                result = subprocess.run(EXE_PTN, input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
                end_time = time.perf_counter()
                print("运行时间: ", round((end_time - start_time)*1000, 2), "ms")
                out = result.stdout.split("\n")
                out = [line.strip() for line in out]
                if result.returncode != "":
                    out.append(str(result.returncode))
                for i in range(len(out)-1, -1, -1):
                    if out[i] == "":
                        out.remove("")
                with open(output_path, 'r') as fout:
                    i = 0
                    for line in fout.readlines():
                        line = line.strip("\r").strip("\n").strip()
                        if line == '':
                            continue
                        if out[i].strip('\r').strip("\n") != line:
                            print("\t\033[31mcase " + str(case)+ " is wrong\033[0m")
                            wrong_cases.append(case)
                            wrong_answers.append(out)
                            break
                        i = i + 1
                
            except Exception as e:
                print(e, end='')
                print('\t\033[31mCodeGen or CodeExecute Fail\033[0m')
                
    for i, wrong_case in enumerate(wrong_cases):
        print("Wrong Case: "+ str(wrong_case))
        print("Wrong Answer: "+ str(wrong_answers[i]))
                
        
for TEST_BASE_PATH in TEST_DIRS:
    testcases = {}
    testcase_list = list(map(lambda x: x.split("."), os.listdir(TEST_BASE_PATH)))
    testcase_list.sort()
    for i in range(len(testcase_list)-1, -1, -1):
        if len(testcase_list[i]) == 1 or testcase_list[i][0] == "":
            testcase_list.remove(testcase_list[i])
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = False
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = testcases[testcase_list[i][0]] | (testcase_list[i][1] == "in")
    evaluate(TEST_BASE_PATH, testcases)