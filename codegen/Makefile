build:
	rm -rf *.o
	rm -rf *.dSYM
	cc -g -O0 *.cpp -o math_lib_generator.o
run:
	./math_lib_generator.o
	cp -rf antiqua_math_generated.h ../antiqua/
