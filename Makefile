LLVMCODE = main
TEST = test

OPTIMIZER = global_optimizer

$(LLVMCODE): $(LLVMCODE).c $(OPTIMIZER).c
	g++ -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c $(OPTIMIZER).c
	g++ $(LLVMCODE).o $(OPTIMIZER).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

$(OPTIMIZER).o: $(OPTIMIZER).c $(OPTIMIZER).h
	gcc -g -I /usr/include/llvm-c-15/ -c $(OPTIMIZER).c

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
