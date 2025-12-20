# Компилятор
CXX = g++
CXXFLAGS = -std=c++17 -pthread -I. -I./tests -I./sequences

# Пути Google Test
GTEST_INC = -I./googletest/googletest/include
GTEST_SRC_DIR = ./googletest/googletest/src

# Файлы Google Test
GTEST_SRC = $(GTEST_SRC_DIR)/gtest-all.cc

# Все исходные файлы
SRCS = main.cpp $(GTEST_SRC)

# Цели
all:
	main
	start

main:
	$(CXX) $(CXXFLAGS) $(GTEST_INC) $(SRCS) -o main.exe

start:
	./main.exe

clean:
	rm -f main.exe

.PHONY: all compile run clean