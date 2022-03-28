all: DiskFileSystem.cpp 
	g++  DiskFileSystem.cpp  -o main
all-GDB:  DiskFileSystem.cpp 
	g++  -g DiskFileSystem.cpp -o main
