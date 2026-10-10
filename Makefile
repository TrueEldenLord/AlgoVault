CXX      = g++
CC       = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
CFLAGS   = -O2 -DSQLITE_THREADSAFE=1
TARGET   = leetcode_db
OBJS     = main.o LinkedList.o sqlite3.o

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) -ldl

main.o: main.cpp LinkedList.h Problem.h sqlite3.h httplib.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

LinkedList.o: LinkedList.cpp LinkedList.h Problem.h
	$(CXX) $(CXXFLAGS) -c LinkedList.cpp -o LinkedList.o

sqlite3.o: sqlite3.c
	$(CC) $(CFLAGS) -c sqlite3.c -o sqlite3.o

clean:
	rm -f $(OBJS) $(TARGET) cache.db

.PHONY: clean
