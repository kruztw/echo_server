all:
	g++  server.cpp -o server -lpthread -std=c++17 -Werror

.PHONY: clean
clean:
	rm ./server
