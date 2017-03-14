
// Async MQTT client cli tool
// Written by David Guillen Fandos <david@davidgf.net>

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mqtt.h"

int main(int argc, char ** argv) {
	// Syntax argv[0] hostname port user pass
	int fdsock = socket(AF_INET , SOCK_STREAM , 0);

	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
 
	if (connect(fdsock , (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("connect failed. Error");
		exit(1);
	}

	bool subscribed = false;
	AsyncMQTTClient client("cliclient_" + std::to_string(getpid()), argv[3], argv[4]);

	while (true) {
		fd_set rfds, wfds, efds;
		FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&efds);
		FD_SET(fdsock, &rfds); FD_SET(fdsock, &efds);

		if (client.hasOutput())
			FD_SET(fdsock, &wfds);

		// Wait for data in and/or out (and err!)
		select(fdsock+1, &rfds, &wfds, &efds, NULL);

		char tmpb[1024];
		int r = recv(fdsock, tmpb, sizeof(tmpb), MSG_DONTWAIT);
		if (r > 0)
			client.inputCallback(std::string(tmpb, r));
		else if (r == 0 || (r < 0 && errno != EWOULDBLOCK)) {
			std::cerr << "read() closed/error in the connection" << std::endl;
			exit(0);
		}

		if (client.isConnected() && !subscribed) {
			subscribed = true;
			if (argc == 7)
				client.publish(argv[5], argv[6], 0);
			else
				client.subscribe("/#", 0);
		}

		std::string topic, value;
		while (client.getMessage(topic, value))
			std::cout << topic << " " << value << std::endl;

		std::string tosend = client.getOutputBuffer(1024);
		int w = send(fdsock, tosend.data(), tosend.size(), MSG_DONTWAIT);
		if (w > 0)
			client.consumeOuput(w);

	}
}
