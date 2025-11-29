CFLAGS = -g -O0 

all: main

main: src/main.c | dist 
	clang $(CFLAGS) src/main.c -o dist/main && cp src/html/index.html dist/html/index.html

dist: 
	mkdir -p dist && mkdir -p dist/html

dist/html: dist
	mkdir -p dist/html


clean: 
	rm -rf dist/*
