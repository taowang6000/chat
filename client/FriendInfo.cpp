/*
 * FriendInfo.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: tao
 */
#include <list>
#include <string>
#include <vector>
#include <iostream>
#include "FriendInfo.h"


FriendInfo::FriendInfo(){
	std::list<Node> friendList;
	std::list<std::string> invi_sent, invi_received;
}

FriendInfo::~FriendInfo(){
	clear();
}

bool FriendInfo::addFriend(std::string & name){
	if (find(name)) return false;
	Node node_temp;
	node_temp.online = false;
	node_temp.connected = false;
	node_temp.message = false;
	node_temp.friendName = name;
	friendList.push_back(node_temp);
	return true;
}

bool FriendInfo::removeFriend(std::string & name){
	if (!find(name)) return false;
	if (isOnline(name)) return false;
	if (isConnected(name)) return false;
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name){
			friendList.erase(itr);
			return true;
		}
	}
	return false;
}

std::vector<std::string> FriendInfo::getOnlineFriends(){
	std::vector<std::string> v;
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->online == true) v.push_back(itr->friendName);
	}
	return v;
}

std::vector<std::string> FriendInfo::getConnectedFriends(){
	std::vector<std::string> v;
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->connected == true) v.push_back(itr->friendName);
	}
	return v;
}

bool FriendInfo::find(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name) return true;
	}
	return false;
}

bool FriendInfo::isOnline(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name) {
			if (itr->online == true) return true;
			else break;
		}
	}
	return false;
}

bool FriendInfo::isMessaged(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name) {
			if (itr->message == true) return true;
			else break;
		}
	}
	return false;
}

bool FriendInfo::isConnected(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name) {
			if (itr->connected == true) return true;
			else break;
		}
	}
	return false;
}

bool FriendInfo::setOnline(std::string & name, std::string & IPaddr, std::string & port){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name){
			itr->online = true;
			itr->IPaddress = IPaddr;
			itr->port = port;
			return true;
		}
	}
	return false;
}

bool FriendInfo::setOffline(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name){
			itr->online = false;
			return true;
		}
	}
	return false;
}

bool FriendInfo::setConnected(std::string & name, int sd){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name){
			itr->connected = true;
			itr->sd = sd;
			return true;
		}
	}
	return false;
}

bool FriendInfo::setDisconnected(int sd){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->sd == sd){
			itr->connected = false;
			return true;
		}
	}
	return false;
}

bool FriendInfo::setMessageOn(std::string &name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name){
			itr->message = true;
			return true;
		}
	}
	return false;
}

bool FriendInfo::setMessageOff(std::string &name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->friendName == name){
			itr->message = false;
			return true;
		}
	}
	return false;
}

void FriendInfo::dump(){
	std::cout << "All friends: ";
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		std::cout << itr->friendName << ' ';
	}
	std::cout << std::endl;
	std::cout << "Online friends: ";
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->online == true) std::cout << itr->friendName << ' ';
	}
	std::cout << std::endl;
	std::cout << "Connected friends: ";
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->connected == true) std::cout << itr->friendName << ' ';
	}
	std::cout << std::endl;
	std::cout << "Friends who have offline messages: ";
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (itr->message == true) std::cout << itr->friendName << ' ';
	}
	std::cout << std::endl;
}

void FriendInfo::clear(){
	size_t size_temp = friendList.size();
	for (int i = 0; i < size_temp; i++){
		friendList.pop_back();
	}
}

int FriendInfo::getSockByName(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (name == itr->friendName){
			return (itr->sd);
		}
	}
	return -1;
}

std::string FriendInfo::getNameBySock(int sd){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (sd == itr->sd){
			return (itr->friendName);
		}
	}
	std::string str;
	return str;
}

std::string FriendInfo::getIPaddr(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (name == itr->friendName){
			return (itr->IPaddress);
		}
	}
	std::string str;
	return str;
}

std::string FriendInfo::getPort(std::string & name){
	for (auto itr = friendList.begin(); itr != friendList.end(); itr++){
		if (name == itr->friendName){
			return (itr->port);
		}
	}
	std::string str;
	return str;
}

bool FriendInfo::isSent(std::string & name){
	for (auto itr = invi_sent.begin(); itr != invi_sent.end(); itr++){
		if (*itr == name) return true;
	}
	return false;
}

bool FriendInfo::isReceived(std::string & name){
	for (auto itr = invi_received.begin(); itr != invi_received.end(); itr++){
		if (*itr == name) return true;
	}
	return false;
}

bool FriendInfo::addSent(std::string & name){
	if (isSent(name)) return false;
	invi_sent.push_back(name);
	return true;
}

bool FriendInfo::addReceived(std::string & name){
	if (isReceived(name)) return false;
	invi_received.push_back(name);
	return true;
}

bool FriendInfo::removeSent(std::string & name){
	if (!isSent(name)) return false;
	invi_sent.remove(name);
	return true;
}

bool FriendInfo::removeReceived(std::string & name){
	if (!isReceived(name)) return false;
	invi_received.remove(name);
	return true;
}



