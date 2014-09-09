CXX=clang++
CPPFLAGS=-std=c++11 -Wall
LDLIBS=-L/usr/local/lib -I/usr/local/include -lboost_filesystem -lboost_system 

all:
		$(CXX) $(CPPFLAGS) $(LDLIBS)  main.cpp fileUtils.cpp -o file_utils
