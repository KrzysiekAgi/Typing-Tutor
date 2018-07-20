all:
	g++ -o main main_corr.cpp -std=c++11 -pthread -lncurses
	./main