all: main

main: src/main.c | dist
	clang src/main.c -o dist/main

dist: 
	mkdir -p dist

clean: 
	rm -rf dist/*
