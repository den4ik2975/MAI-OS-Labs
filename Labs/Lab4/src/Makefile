all: program1 program2 lib_pr2_1.so lib_pr2_2.so clean

# Program 1
program1: program1.o lib_pr2_1.so
	g++ -o program1 program1.o -L. -l_pr2_1 -Wl,-rpath,.

program1.o: program1.cpp
	g++ -c program1.cpp

int_rect.o: int_rect.cpp
	g++ -c int_rect.cpp

tr_bin.o: tr_bin.cpp
	g++ -c tr_bin.cpp

# Program 2
program2: program2.o
	g++ -o program2 program2.o -ldl

program2.o: program2.cpp
	g++ -c program2.cpp 

lib_pr2_1.so: int_rect.o tr_bin.o
	g++ -shared -o lib_pr2_1.so int_rect.o tr_bin.o

lib_pr2_2.so: int_trap.o tr_tri.o
	g++ -shared -o lib_pr2_2.so int_trap.o tr_tri.o

int_trap.o:
	g++ -c int_trap.cpp

tr_tri.o:
	g++ -c tr_tri.cpp

# Clean
clean:
	rm -f *.o