# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 1030

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

all: server subscriber

# Compileaza server.cpp
server: server.cpp
	g++ server.cpp -o server -Wall

# Compileaza subscriber.cpp
subscriber: subscriber.cpp
	g++ subscriber.cpp -o subscriber -Wall

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
