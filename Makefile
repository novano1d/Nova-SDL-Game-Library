all:
	g++ -Isrc/Include -Lsrc/lib -o main main.cpp sdlgame.cpp -lmingw32 -lSDL2main -lSDL2