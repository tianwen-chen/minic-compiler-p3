LLVMCODE = main
TEST = test

GLOBAL_OPTIMIZER = global_optimizer
GLOBAL_OPTMIZER_SRCS = local_optimizer.c DArray.c
GLOBAL_OPTIMIZER_OBJS = $(GLOBAL_OPTIMIZER).o local_optimizer.o DArray.o
DEPS_GLOBAL_OPTIMIZER = local_optimizer.h DArray.h

MAIN_SRCS = llvm_parser.c $(GLOBAL_OPTIMIZER).c
MAIN_OBJS = main.o
DEPS_MAIN = llvm_parser.h  global_optimizer.h $(DEPS_GLOBAL_OPTIMIZER)

local_optimizer.o: local_optimizer.c local_optimizer.h DArray.h
	g++ -g -I /usr/include/llvm-c-15/ -c local_optimizer.c

$(GLOBAL_OPTIMIZER_OBJS): $(GLOBAL_OPTIMIZER_SRCS) $(DEPS_GLOBAL_OPTIMIZER)
	g++ -g -I /usr/include/llvm-c-15/ -c $(GLOBAL_OPTIMIZER_SRCS)

$(MAIN_OBJS): $(MAIN_SRCS) $(DEPS_MAIN)
	g++ -g -I /usr/include/llvm-c-15/ -c $(MAIN_SRCS)

$(LLVMCODE): $(LLVMCODE).c $(MAIN_OBJS) $(GLOBAL_OPTIMIZER_OBJS)
	g++ -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c $(MAIN_OBJS) $(GLOBAL_OPTIMIZER_OBJS)
	g++ $(LLVMCODE).o $(LOCAL_OPTIMIZER).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o