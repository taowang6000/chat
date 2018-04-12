/*
 * ClientConfig.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: tao
 */
#include <list>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "ClientConfig.h"


ClientConfig::ClientConfig(){
	std::list<std::pair<std::string,std::string>> configList;
}

ClientConfig::~ClientConfig(){
	clear();
}

void ClientConfig::clear(){
	if (configList.size() != 0){
		size_t list_size = configList.size();
		for (int i = 0; i < list_size; i++)
			configList.pop_back();
	}
}

bool ClientConfig::load(const char *filename){
	std::ifstream fin;
	std::string wholeline;

	fin.open(filename, std::ios_base::in);
	if (!fin) {
		return false;
	}

	while (getline(fin, wholeline)){
		std::pair<std::string, std::string> tokens;
		tokens = ClientConfig::parse(wholeline);
		if (find(tokens.first)){
			std::cout << "Error: Duplicate keyword '" << tokens.first
					<< "', duplicate keyword ignored" << std::endl;
		}
		else{
			configList.push_back(tokens);
		}
	}
	fin.close();
	return true;
}

bool ClientConfig::find(const std::string & key) const{
	for (auto itr = configList.begin(); itr != configList.end(); itr++){
		if (itr->first == key)
			return true;
	}
	return false;
}

std::pair<std::string, std::string> ClientConfig::parse(std::string & line){
	//replace ":" with " "
	for (auto itr = line.begin(); itr != line.end(); itr++){
		if (*itr == ':')
			*itr = ' ';
	}

	std::pair<std::string, std::string> parsed_str;
	std::stringstream ss;
	ss << line;
	ss >> parsed_str.first;
	ss >> parsed_str.second;
	return parsed_str;
}

std::string ClientConfig::getvalue(const std::string & key) const{
	for (auto itr = configList.begin(); itr != configList.end(); itr++){
		if (itr->first == key)
			return itr->second;
		}
	std::string str;
	return (str);
}





