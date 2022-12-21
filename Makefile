.PHONY: all

all: build/buildtool
	./build/buildtool

build/buildtool: buildtool.cpp
	mkdir -p build
	c++ -g -o build/buildtool -std=c++2a buildtool.cpp

