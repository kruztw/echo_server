all:
	g++  server.cpp -o server -lpthread -std=c++17 -Werror -ggdb
	g++  pressure_test.cpp -o pressure_test -lpthread -std=c++17 -Werror -ggdb

.PHONY: clean
clean:
	rm ./server
	rm ./pressure_test
