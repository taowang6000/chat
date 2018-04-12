/*
 * ActiveUser.cpp
 *
 *  Created on: Oct 28, 2016
 *      Author: tao
 */
#include <list>
#include <string>
#include <vector>
#include <iostream>
#include "ActiveUser.h"

//using namespace std;

ActiveUser::ActiveUser(){
	std::list<Node> activeLists;
}

ActiveUser::~ActiveUser(){
	clear();
}

bool ActiveUser::contains(const std::string & name){
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (itr->username == name) return true;
	}
	return false;
}

bool ActiveUser::insert(const std::string & name){
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (itr->username == name) return false;
	}
	Node	node_temp;
	node_temp.username = name;
	activeLists.push_back(node_temp);
	return true;
}

bool ActiveUser::update(const std::string & name,const std::string & IPaddr,
		const std::string & port, const int sd){
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (itr->username == name && itr->IPaddress == IPaddr
				&& itr->portNumber == port && itr->socket == sd)
					return false;
	}

	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (itr->username == name){
			itr->IPaddress = IPaddr;
			itr->portNumber = port;
			itr->socket = sd;
			return true;
		}
	}

	Node node_temp;
	node_temp.username = name;
	node_temp.IPaddress = IPaddr;
	node_temp.portNumber = port;
	node_temp.socket = sd;
	activeLists.push_back(node_temp);
	return true;
}

bool ActiveUser::removeByName(const std::string & name){
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (itr->username == name){
			activeLists.erase(itr);
			return true;
		}
	}
	return false;
}

bool ActiveUser::removeBySocket(const int sd){
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (itr->socket == sd){
			activeLists.erase(itr);
			return true;
		}
	}
	return false;
}

void ActiveUser::clear(){
	if (activeLists.size() != 0){
		size_t list_size = activeLists.size();
		for (int i = 0; i < list_size; i++)
			activeLists.pop_back();
	}
}

void ActiveUser::dump(){
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		std::cout << itr->username << " " << itr->IPaddress
				<< " " << itr->portNumber << " " << itr->socket;
		std::cout << std::endl;
	}
}

std::vector<std::string> ActiveUser::getActiveUsers(){
	std::vector<std::string> users;
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		users.push_back(itr->username);
	}
	return users;
}

std::string ActiveUser::getLocations(std::vector<std::string> & names){

	std::string locations;
	if (names.size() == 0) return locations;
	for (int i = 0; i < names.size(); i++){
		for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
			if (names[i] == itr->username){
				locations = locations + itr->username + ' ' + itr->IPaddress + ' '
						+ itr->portNumber + ' ';
			}
		}
	}
	if (locations.size() == 0) return locations;
	locations.pop_back();		//delete the last ' '
	return locations;
}

std::string ActiveUser::getSingleLocation(std::string & name){
	std::string location;
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (name == itr->username){
			location += itr->username;
			location.push_back(' ');
			location += itr->IPaddress;
			location.push_back(' ');
			location += itr->portNumber;
			return location;
		}
	}
	return location;	//name not found
}

std::vector<int> ActiveUser::getSocks(std::vector<std::string> &names){
	std::vector<int> sockets;
	for (int i = 0; i < names.size(); i++){
		for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
			if (names[i] == itr->username){
				sockets.push_back(itr->socket);
			}
		}
	}
	return sockets;
}

std::string ActiveUser::getNameBySock(int sd){
	std::string name;
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (sd == itr->socket){
			name =itr->username;
			return name;
		}
	}
	return name;
}

int ActiveUser::getSockByName(std::string & name){
	int socket;
	for (auto itr = activeLists.begin(); itr != activeLists.end(); itr++){
		if (name == itr->username){
			socket = itr->socket;
			return socket;
		}
	}
	return -1;
}



