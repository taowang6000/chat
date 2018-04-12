/*
 * AppProt.h
 *
 *  Created on: Oct 29, 2016
 *      Author: tao
 */

#ifndef APPPROT_H_
#define APPPROT_H_

#include <stdlib.h>			//itoa
#include <string>			//stoi
#include <vector>

//using namespace std;
/* Behaving as the server, read the client's information from buffer, decide
 * what decision should be made, and gives appropriate response.
 *
 * information format(between server and client):
 * byte 0-1(2 bytes): message type
 * byte 2-3(2 bytes): payload length
 * byte 4-after: message payload
 */

/*
 * Explanation of message type, from server to client
 * 100: add a friend's location, payload:name, IPaddr, port
 * 200: login ACK, no payload
 * 300: locations of the client's friends online, payload:string of
 * 		(name, IPaddr, port)
 * 400: a friend is offline(with a name as payload)
 * 500: login NAK, no payload
 * 600: forwarding invitation, payload: dest_name, source_name, message
 * 700: forwarding invitation acceptance, dest_name, source_name, message
 * 800: register ACK, no payload
 * 900: register NAK, no payload
 * 1000: friend list of the client, payload: friend_name1, friend_name2 ...
 * 1100: friends' names who have offline messages, payload: name1 name2 ...
 * 1200: offline message, payload: name(message sender) message
 */

/*
 * Explanation of message type, from client to server
 * 101: registration, payload: user-name and password
 * 201: login, payload: user-name, password
 * 301: add client's location, payload: name, IPaddr, port
 * 401: logout, without payload
 * 601: invitation, payload:dest_name, source_name, message
 * 701: invitation acceptance, payload: dest_name, source_name, message
 * 801: get offline messages, payload: username, friendname
 * 901: send a offline message, payload: username, friendname, message
 */

/*
 * Explanation of message type, between clients
 * 102: message, payload: sender's name, message body
 * 202: file transfer starting notice, payload : sender's name, file name
 * 302: piece of file body, payload: a piece of file
 * 402: file transfer ending notice, payload: sender's name, file name
 * 502: agree to receive, no payload
 * 602: reject to receive, no payload
 */

class AppProt{
public:
	AppProt();
	//AppProt(char * buf, int length);
	~AppProt();

	void readBuffer(char *buf, int length);
	void writeSock(int sd);

	uint16_t getType();
	uint16_t getLength();
	std::vector<std::string> getPayload();
	std::string getRawPayload();

	void setType(uint16_t type);
	void setLengthAndPayload(std::string payload);

	void clear();

private:
	char * bufRead;
	int currentReadSize;		//real data size in bufRead
	char * bufToWrite;

	/* a general parsing code to avoid the weakness of stringstream method
	 * (unpredictable length of string)
	 */
	std::vector<std::string> parseToken(std::string & line);

};




#endif /* APPPROT_H_ */
