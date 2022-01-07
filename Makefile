all:
	g++  server.cpp -o server -lpthread

.PHONY: clean
clean:
	rm ./server
