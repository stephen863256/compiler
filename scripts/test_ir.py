import os
import subprocess
import time

EXE_PATH = os.path.abspath("build/SysY")
LIB_PATH = os.path.abspath("src/io/sylib.c")


TEST_DIRS = [
    "supplement/functional",
    #"supplement/hidden_functional",
    #"supplement/perf",
    #"supplement/func",
    "supplement/performance"#,
     #"supplement/final_performance",
]


def evaluate(test_base_path, testcases, optimize):
    print("===========TEST START==============")
    print("now in {}".format(test_base_path))
    
    wrong_cases = []
    wrong_answers = []
    
    for index, case in enumerate(testcases):
        print('Case %s:'% case, end='\n')
        test_path = os.path.join(test_base_path, case)
        sy_path = test_path + ".sy"
        ll_path = test_path + ".ll"
        input_path = test_path + ".in"
        output_path = test_path + ".out"
        exe_path = test_path + ".exe"


        need_input = testcases[case]
        #ASM_GEN_PTN = ["SysY",sy_path,"-S", "-o", asm_path]

        IR_GEN_PTN = ["SysY",sy_path,"-emit-llvm", "-o", ll_path,"-mem2reg"] 
        EXE_GEN_PTN = ["clang", optimize, "-o", exe_path, ll_path, LIB_PATH]
        EXE_PTN = [exe_path]
        
        
        irbuilder_result = subprocess.run(IR_GEN_PTN, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if irbuilder_result.returncode != 0:
            wrong_cases.append(case)
            wrong_answers.append([])
            
        if irbuilder_result.returncode == 0:
            input_option = None
            if need_input:
                with open(input_path, "r") as fin:
                    input_option = fin.read()
            #input_file = open(input_path, 'r') if os.path.exists(input_path) else None  
            try:
                res = subprocess.run(EXE_GEN_PTN, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                if res.returncode != 0:
                    print(res.stderr, end='')
                    print('\t\033[31mClang Execute Fail\033[0m')
                    continue
                start_time = time.perf_counter()
                result = subprocess.run(EXE_PTN, input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                #result = subprocess.run(EXE_PTN, stdin=input_file, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,timeout=5)
                end_time = time.perf_counter()
                print("运行时间: ", round((end_time - start_time)*1000, 2), "ms")
                out = result.stdout.split("\n")
                out = [line.strip() for line in out]
                if result.returncode != "":
                    out.append(str(result.returncode))
                for i in range(len(out)-1, -1, -1):
                    if out[i] == "":
                        out.remove("")
                #print(out)
                with open(output_path, 'r') as fout:
                    i = 0
                    for line in fout.readlines():
                        line = line.strip("\r").strip("\n").strip()
                        if line == '':
                            continue
                        if out[i].strip('\r').strip("\n") != line:
                            print('\t\033[31mcase ' + str(case) + ' is wrong\033[0m')
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
    optimize = "-O0"
    for i in range(len(testcase_list)-1, -1, -1):
        if len(testcase_list[i]) == 1 or testcase_list[i][0] == "":
            testcase_list.remove(testcase_list[i])
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = False
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = testcases[testcase_list[i][0]] | (testcase_list[i][1] == "in")
    evaluate(TEST_BASE_PATH, testcases, optimize)

