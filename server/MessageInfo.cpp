/*
 * MessageInfo.cpp
 *
 *  Created on: Nov 21, 2016
 *      Author: tao
 */
#include <list>
#include <string>
#include <vector>
#include <iostream>
//#include <sstream>
#include "MessageInfo.h"

MessageInfo::MessageInfo(){
	std::list<std::list<Node>> messageLists;
}

MessageInfo::~MessageInfo(){
	clear();
}

void MessageInfo::addMessage(std::string & username, std::string & friendname, std::string & message){
	Node tempNode;
	std::list<Node> tempList;

	for (auto itr = messageLists.begin(); itr != messageLists.end(); itr++){
		if (itr->front().name == username){		//user found
			auto itrTemp = (*itr).begin();
			itrTemp++;
			for (auto in_itr = itrTemp; in_itr != (*itr).end(); in_itr++){
				if (in_itr->name == friendname){ //user and friend found, just add an extra message
					in_itr->messages.push_back(message);
					return;
				}
			}
			tempNode.name = friendname;	//user found, but friend not found, add a friend node
			tempNode.messages.push_back(message);
			(*itr).push_back(tempNode);
			return;
		}
	}
	//neither user nor friend is found, add a user list with two nodes, one for the user itself(without
	//message list), the other for the friend
	tempNode.name = username;
	tempList.push_back(tempNode);
	tempNode.name = friendname;
	tempNode.messages.push_back(message);
	tempList.push_back(tempNode);
	messageLists.push_back(tempList);
}

bool MessageInfo::removeMessages(std::string & username, std::string friendname){
	if (messageLists.size() == 0) return false;
	bool messages_removed = false;
	for (auto itr = messageLists.begin(); itr != messageLists.end(); itr++){
		if (itr->front().name == username){			//user found
			auto itrTemp = (*itr).begin();
			itrTemp++;
			for (auto in_itr = itrTemp; in_itr != (*itr).end(); in_itr++){
				if (in_itr->name == friendname){	//friend found
					(*itr).erase(in_itr);			//delete the friend node
					messages_removed = true;
					in_itr--;
				}
			}
		}
	}
	for (auto itr = messageLists.begin(); itr != messageLists.end(); itr++){
		if ((*itr).size() == 1){		//all the user's friends have been deleted
			messageLists.erase(itr);
			itr--;
		}
	}
	return messages_removed;
}

std::list<std::string> MessageInfo::getMessages(std::string & username, std::string & friendname){
	std::list<Node> listTemp;
	for (auto itr = messageLists.begin(); itr != messageLists.end(); itr++){
		if (itr->front().name == username){			//user found
			auto itrTemp = (*itr).begin();
			itrTemp++;
			for (auto in_itr = itrTemp; in_itr != (*itr).end(); in_itr++){
				if (in_itr->name == friendname){
					return in_itr->messages;
				}
			}
		}
	}
	std::list<std::string> messages;
	return messages;
}


bool MessageInfo::find(std::string &name){
	if (messageLists.size() == 0) return false;
	for (auto itr = messageLists.begin(); itr != messageLists.end(); itr++){
		if (itr->front().name == name) return true;
	}
	return false;
}

void MessageInfo::clear(){
	size_t tempSize = messageLists.size();
	if (tempSize == 0) return;
	for (int i = 0; i < tempSize; i++){
		messageLists.pop_back();
	}
}

void MessageInfo::dump(){
	if (messageLists.size() == 0) return;
	std::list<Node> listTemp;
	for (auto itr = messageLists.begin(); itr != messageLists.end(); itr++){
		listTemp = *itr;
		std::cout << listTemp.front().name << "'messages: \n";
		if (listTemp.size() > 1){
			auto itrTemp = listTemp.begin();
			itrTemp++;
			for (auto user_itr = itrTemp; user_itr != listTemp.end(); user_itr++){
				std::cout << "From " << user_itr->name << ": \n";
				for (auto in_itr = user_itr->messages.begin();
						in_itr != user_itr->messages.end(); in_itr++){
					std::cout << *in_itr << std::endl;
				}
				std::cout << std::endl;
			}
		}
	}
}

std::string MessageInfo::getNames(std::string & username){
	std::string str;
	if (messageLists.size() == 0) return str;
	for (auto itr = messageLists.begin(); itr != messageLists.end(); itr++){
		if (itr->front().name == username){
			auto itrTemp = (*itr).begin();
			itrTemp++;
			for (auto in_itr = itrTemp; in_itr != (*itr).end(); in_itr++){
				str = str + in_itr->name + ' ';
			}
			if (str.size() > 0) str.pop_back();
			return str;
		}
	}

	return str;
}


