
// Async MQTT client
//
// Written by David Guillen Fandos <david@davidgf.net>
// Use it under the public domain

#include <algorithm>
#include <tuple>

#include "mqtt.h"

// Return a 2-byte buffer with a big endian 16b int + data
std::string strWH(std::string s) {
	uint8_t hdr[2] = {(uint8_t)(s.size() >> 8), (uint8_t)s.size()};
	return std::string((char*)&hdr[0], 2) + s;
}

std::string decWHstr(uint8_t *ptr, unsigned size) {
	uint16_t count = (((uint16_t)ptr[0]) << 8) | ptr[1];
	if (count + 2 <= size)
		return std::string((char*)&ptr[2], count);
	return "";
}

// Return a variable encoded integer
std::string encSize(unsigned c) {
	std::string ret;
	do {
		uint8_t d = c & 0x7F;
		c = c >> 7;
		if (c > 0)
			d |= 0x80;
		ret.push_back(d);
	} while(c);
	return ret;
}

// Decode a var-length buffer and return
// the number and the number of consumed bytes
std::pair<unsigned, unsigned> decSize(uint8_t *inb, unsigned maxsize) {
	unsigned ret = 0;
	for (unsigned i = 0; i < std::min(4U, maxsize); i++) {
		ret <<= 7;
		uint8_t d = inb[i] & 0x7F;
		ret |= d;
		if ((inb[i] & 0x80) == 0)
			return std::make_pair(ret, i+1);
	}
	// This means either premature end of stream or more than 4 bytes long
	return std::make_pair(0, 0);
}

AsyncMQTTClient::AsyncMQTTClient(std::string clientid)
 : clientid(clientid), keepalive(30), ccode(csConnecting) {

	sendConnect();
}

AsyncMQTTClient::AsyncMQTTClient(std::string clientid, std::string user, std::string pass)
 : clientid(clientid), user(user), pass(pass), keepalive(30), ccode(csConnecting) {

	sendConnect();
}

// Queue data in the buffer and flag data available
void AsyncMQTTClient::writeOutput(std::string p) {
	this->outbuffer += p;
}

// Get pending messages
bool AsyncMQTTClient::getMessage(std::string &topic, std::string &value) {
	if (messages.size()) {
		topic = messages.front().first;
		value = messages.front().second;
		messages.pop_front();
		return true;
	}
	return false;
}

void AsyncMQTTClient::inputCallback(std::string freshdata) {
	inbuffer += freshdata;
	while (true) {
		int r = process();
		if (r == 0)
			break;
		inbuffer = inbuffer.substr(r);
	}
}

// Process input messages
unsigned AsyncMQTTClient::process() {
	if (inbuffer.size() < 3)
		return 0; // Requires 1b header + 1b length + 1b payload

	uint8_t * uptr = (uint8_t*)inbuffer.data();

	// Parse var length header
	unsigned psize, cbytes;
	std::tie(psize, cbytes) = decSize(&uptr[1], inbuffer.size() - 1);
	if (cbytes == 0) {
		ccode = csConStreamError;
		return 0; // Stream error!
	}

	if (1 + cbytes + psize > inbuffer.size())
		return 0; // Not enough data yet

	uint8_t ctype = uptr[0] >> 4;
	uint8_t *payload = (uint8_t*)&uptr[1 + cbytes];

	switch (ctype) {
	case 2: // Conn ACK
		if (isConnected())
			ccode = csConStreamError;
		else {
			if (psize == 2)
				ccode = (ConnectionCode)payload[1];
			else
				ccode = csConErrorUnknown;
		}
		break;
	case 3: // Publish message
		if (psize >= 2) {
			std::string topic = decWHstr(payload, psize);
			unsigned rem = psize - topic.size() - 2;
			std::string value((char*)&payload[topic.size()+2], rem);
			messages.push_back(std::make_pair(topic, value));
		}
		break;
	};

	// Consume these bytes
	return 1 + cbytes + psize;
}

// Send the connect message
void AsyncMQTTClient::sendConnect() {
	std::string header("\x10", 1);             // Connect request
	std::string protoname("\x00\x04MQTT", 6);  // MQTT 3.1.1
	std::string protolevel("\x04", 1);         //

	uint8_t flags = 0x02; // clean session, QoS 0 only for now
	if (user.size())
		flags |= 0x80;
	if (pass.size())
		flags |= 0x40;
	uint8_t keepalive[2] = {(uint8_t)(this->keepalive >> 8), 
	                        (uint8_t)this->keepalive};

	std::string pflags((char*)&flags, 1);
	std::string pkalive((char*)keepalive, 2);

	std::string packet = protoname + protolevel +
	                     pflags + pkalive + strWH(clientid);
	if (user.size())
		packet += strWH(user);
	if (pass.size())
		packet += strWH(pass);

	std::string encoded_size = encSize(packet.size());

	std::string out = header + encoded_size + packet;

	writeOutput(out);
}

void AsyncMQTTClient::subscribe(std::string topic, uint8_t qos) {
	std::string header("\x82", 1);       // Subscribe request
	std::string msgid("\x00\x01", 2);

	// Request the topic with QoS = 0
	std::string stopic = strWH(topic) + std::string("\0", 1);
	std::string payload = msgid + stopic;

	std::string out = header + encSize(payload.size()) + payload;

	writeOutput(out);
}

// Send a publish message
void AsyncMQTTClient::publish(std::string topic, std::string payload, bool retain) {
	// Publish header
	std::string header = retain ? std::string("\x31", 1) : std::string("\x30", 1);
	// TODO
}

