.PHONY: system

system: build/buildtool
	./build/buildtool

build/buildtool: buildtool.cpp
	c++ -o build/buildtool -std=c++2a -fsanitize=undefined buildtool.cpp

