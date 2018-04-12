/*
 * ClientProt.h
 *
 *  Created on: Nov 8, 2016
 *      Author: tao
 */

#ifndef CLIENTPROT_H_
#define CLIENTPROT_H_
#include <stdlib.h>			//itoa
#include <string>			//stoi
#include <vector>


/* Behaving as the client, read the server and other clients' information from
 *  buffer, decide what decision should be made, and gives appropriate response.
 */

class ClientProt{
public:
	ClientProt();
	//AppProt(char * buf, int length);
	~ClientProt();

	// read from buffer, to "bufRead"
	void readBuffer(char *buf, int length);
	// write the content of "bufToWrite" into the socket
	void writeSock(int sd);

	uint16_t getType();
	uint16_t getLength();
	std::vector<std::string> getPayload();
	std::string getRawPayload();

	void setType(uint16_t type);
	void setLength(uint16_t length);
	void setLengthAndPayload(std::string payload);

	void clear();

private:
	char * bufRead;
	int currentReadSize;		//real data size in bufRead
	char * bufToWrite;

	std::vector<std::string> parseToken(std::string & line);
};




#endif /* CLIENTPROT_H_ */
