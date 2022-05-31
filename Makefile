all:
	g++ -o nes main.cc src/*.cc src/cart/*.cc -I include/ -std=c++17
