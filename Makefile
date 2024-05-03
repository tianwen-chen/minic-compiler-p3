LLVMCODE = main
TEST = test

LOCAL_OPTIMIZER = global_optimizer

$(LLVMCODE): $(LLVMCODE).c $(LOCAL_OPTIMIZER).c
	g++ -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c $(LOCAL_OPTIMIZER).c
	g++ $(LLVMCODE).o $(LOCAL_OPTIMIZER).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

$(LOCAL_OPTIMIZER).o: $(LOCAL_OPTIMIZER).c $(LOCAL_OPTIMIZER).h
	gcc -g -I /usr/include/llvm-c-15/ -c $(LOCAL_OPTIMIZER).c

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
