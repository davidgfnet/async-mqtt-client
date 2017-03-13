
// Async MQTT client
//
// Written by David Guillen Fandos <david@davidgf.net>
// Use it under the public domain

#ifndef ASYNC_MQTT_CLIENT_H__
#define ASYNC_MQTT_CLIENT_H__

#include <string>
#include <list>
#include <tuple>

enum ConnectionCode {
	csConnected          = 0,
	csRefusedVer         = 1,
	csRefusedIDRej       = 2,
	csRefusedNAvail      = 3,
	csRefusedBadUserPass = 4,
	csRefusedNotAuth     = 5,

	// Interal error codes
	csConStreamError     = 253,  // Some error in the stream decoding
	csConErrorUnknown    = 254,  // Some unknown error in the connection ACK
	csConnecting         = 255   // Still connecting, keep on waiting
};


class AsyncMQTTClient {
public:

	AsyncMQTTClient(std::string clientid);
	AsyncMQTTClient(std::string clientid, std::string user, std::string pass);

	void publish(std::string topic, std::string payload, bool retain);
	void subscribe(std::string topic, uint8_t qos);
	void unsubscribe(std::string topic, uint8_t qos);

	bool isConnected() const { return ccode == csConnected; }

	bool getMessage(std::string &topic, std::string &value);

	// Interface to the network
	bool hasOutput() const { return outbuffer.size() > 0; }
	std::string getOutputBuffer(unsigned maxsize) const {
		if (maxsize >= outbuffer.size())
			return outbuffer;
		return outbuffer.substr(0, maxsize);
	}
	void consumeOuput(unsigned ncount) { outbuffer = outbuffer.substr(ncount); }
	void inputCallback(std::string freshdata);


protected:

	std::string clientid, user, pass;
	unsigned keepalive;
	ConnectionCode ccode;

	std::string outbuffer, inbuffer;
	std::list < std::pair<std::string, std::string> > messages;

	void sendConnect();
	void writeOutput(std::string p);
	unsigned process();
};

#endif

