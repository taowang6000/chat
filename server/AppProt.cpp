/*
 * AppProt.cpp
 *
 *  Created on: Nov 1, 2016
 *      Author: tao
 */

#include <netdb.h>			//htons(), ntohs()
#include <string.h>			//memset()
#include <unistd.h>			//write, read

#include <list>
#include <string>
#include <vector>
#include <iostream>
#include "AppProt.h"

#define BUFFERSIZE 1024

//using namespace std;

AppProt::AppProt( ){
	bufRead = new char[BUFFERSIZE];
	bufToWrite = new char[BUFFERSIZE];
	currentReadSize = 0;
}

AppProt::~AppProt(){
	delete []bufRead;
	delete []bufToWrite;
}

void AppProt::readBuffer(char *buf, int length){
	if (BUFFERSIZE < length){
		std::cout << "BUFFERSIZE too small to read" << std::endl;
		return;
	}
	for (auto i = 0; i < length; i++){
		bufRead[i] = buf[i];
	}
	currentReadSize = length;
}

void AppProt::writeSock(int sd){
	write(sd,bufToWrite, BUFFERSIZE);
}

uint16_t AppProt::getType(){
	uint16_t i;
	if (currentReadSize < 4){
		std::cout << "No type area" << std::endl;
		return 0;
	}
	memcpy(&i, bufRead, sizeof(i));
	return ntohs(i);
}

uint16_t AppProt::getLength(){
	uint16_t i;
	if (currentReadSize < 8){
		std::cout << "No length area" << std::endl;
		return 0;
	}
	memcpy(&i, bufRead + 4, sizeof(i));
	return ntohs(i);;
}

std::vector<std::string> AppProt::getPayload(){
	std::vector<std::string> v;
	std::string str_in, token;
	if (currentReadSize < 9){
		std::cout << "No payload area" << std::endl;
		return v;
	}
	for (int i = 8; i < currentReadSize; i++){
		str_in.push_back(bufRead[i]);
	}
	v = parseToken(str_in);
	return v;
}

std::string AppProt::getRawPayload(){
	std::string str_in;
	if (currentReadSize < 9){
		std::cout << "No payload area" << std::endl;
		return str_in;
	}
	for (int i = 8; i < currentReadSize; i++){
		str_in.push_back(bufRead[i]);
	}
	return str_in;
}

void AppProt::setType(uint16_t type){
	uint16_t i;
	i = htons(type);
	memcpy(bufToWrite, &i, sizeof(i));
}

void AppProt::setLengthAndPayload(std::string payload){
	if (payload.size() == 0) return;
	uint16_t i, length;
	length = (uint16_t)payload.size();
	i = htons(length);
	memcpy(bufToWrite + 4, &i, sizeof(i));
	char * buf_temp;
	buf_temp = (char *)payload.c_str();
	memcpy(bufToWrite + 8,buf_temp, strlen(buf_temp));
}

void AppProt::clear(){
	currentReadSize = 0;
	memset(bufRead, 0 ,BUFFERSIZE);
	memset(bufToWrite, 0, BUFFERSIZE);
}

std::vector<std::string> AppProt::parseToken(std::string & line){
	std::vector<std::string> v;
	std::string str;

	for (int i = 0; i < line.size(); i++){
		if (line[i] == ' '){
			if (str.size() != 0){
				v.push_back(str);
				str.clear();
			}
		}
		else if (line[i] != '\0'){
			str.push_back(line[i]);
		}
	}
	v.push_back(str);
	return v;
}




