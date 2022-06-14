all:
	g++ -Wall -o nes main.cc src/*.cc src/cart/*.cc -I include/ -lSDL2 -std=c++17 -Ofast

debug:
	g++ -Wall -DDEBUG -o nes main.cc src/*.cc src/cart/*.cc src/debug/*.cc -I include/ -lcurses -lSDL2 -std=c++17
