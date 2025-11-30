CFLAGS = -g -O0 -Wall

all: main

main: src/main.c | dist 
	clang $(CFLAGS) src/main.c -o dist/main && cp src/html/index.html dist/html/index.html && cp src/html/not-found.html dist/html/not-found.html

dist: 
	mkdir -p dist && mkdir -p dist/html

dist/html: dist
	mkdir -p dist/html


clean: 
	rm -rf dist/*
