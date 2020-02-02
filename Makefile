all: client server

client: messenger_client.cpp messenger_client.h config.h
	g++ -g -pthread messenger_client.cpp -o client

server: messenger_server.cpp messenger_server.h config.h
	g++ -g -pthread messenger_server.cpp -o server

clean:
	rm -f client server 
