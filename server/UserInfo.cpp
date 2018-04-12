/*
 * UserInfo.cpp
 *
 *  Created on: Oct 26, 2016
 *      Author: tao
 */
#include <list>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "UserInfo.h"


UserInfo::UserInfo(){
	std::list<std::list<std::string>> userLists;
}

UserInfo::~UserInfo(){
	clear();
}

std::list<std::string> UserInfo::parse(std::string & line){
	//replace "|,:" with " "
	for (auto itr = line.begin(); itr != line.end(); itr++){
		if (*itr == '|' || *itr == ';')
			*itr = ' ';
	}

	return parseToken(line);
}

bool UserInfo::load(const char *filename){
	std::ifstream fin;
	std::string wholeline;

	fin.open(filename, std::ios_base::in);
	if (!fin) {
		return false;
	}

	while (getline(fin, wholeline)){
		std::list<std::string> tokens;
		tokens = UserInfo::parse(wholeline);
		if (find(tokens.front())){
				std::cout << "Error: Duplicate user '" << tokens.front()
						<< "', duplicate user ignored" << std::endl;
		}
		else{
				userLists.push_back(tokens);
		}
	}
	fin.close();
	return true;
}

bool UserInfo::find(const std::string & name) const{
	for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
		if (itr->front() == name)
			return true;
	}
	return false;
}

bool UserInfo::match(const std::string & name, const std::string & password) const{
	for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
		if (itr->front() == name){
			auto whichlist = *itr;
			whichlist.pop_front();
			if (!password.compare(whichlist.front())){
				return true;
			}
		}
	}
	return false;
}

bool UserInfo::addUser(const std::string & name, const std::string password){
	if (find(name))	return false;
	std::list<std::string> temp_list;
	temp_list.push_back(name);
	temp_list.push_back(password);
	userLists.push_back(temp_list);
	return true;
}

bool UserInfo::removeUser(const std::string &name){
	for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
		if (itr->front() == name){
			userLists.erase(itr);
			return true;
		}
	}
	return false;
}

bool UserInfo::addFriend(const std::string & username, const std::string & friendname){
	for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
		if (itr->front() == username){
			if ((*itr).size() == 2) {
				(*itr).push_back(friendname);
				return true;
			}
			else{
				auto itr_temp = (*itr).begin();
				itr_temp++;
				itr_temp++;
				for (auto user_itr = itr_temp; user_itr != (*itr).end(); user_itr++){
					if (*user_itr == friendname){
						std::cout << "friend already exists" << std::endl;
						return false;
					}
				}
				(*itr).push_back(friendname);
				return true;
			}
		}
	}
	std::cout << "User not found " << std::endl;
	return false;
}

bool UserInfo::removeFriend(const std::string & username, const std::string & friendname){
	for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
		if (itr->front() == username){
			auto itr_temp = itr->begin();
			itr_temp++;
			itr_temp++;
			for (auto user_itr = itr_temp; user_itr != itr->end(); user_itr++){
				if (*user_itr == friendname){
					itr->erase(user_itr);
					return true;
				}
			}
		}
	}
	return false;
}

void UserInfo::dump(){
	for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
		for (auto user_itr = itr->begin(); user_itr != itr->end(); user_itr++){
			std::cout << *user_itr << " ";
		}
		std::cout << std::endl;
	}
}

void UserInfo::clear(){
	if (userLists.size() != 0){
		size_t lists_size = userLists.size();
		for (int i = 0; i < lists_size; i++)
			userLists.pop_back();
	}
}

bool UserInfo::write_to_file(const char *filename){
	std::ofstream fout;

	fout.open(filename);
	if (!fout)	return false;
	if (userLists.size() != 0) {
		for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
			int i = 0;
			for (auto user_itr = itr->begin(); user_itr != itr->end(); user_itr++){
				fout << *user_itr;
				i++;
				if (i == itr->size())	fout << std::endl;
				else if (i < 3) fout << "|";
				else fout << ";";
			}
		}
	}
	fout.close();
	return true;
}

std::vector<std::string> UserInfo::getFriends(std::string & username){
	std::vector<std::string> friendNames;
	for (auto itr = userLists.begin(); itr != userLists.end(); itr++){
			if (itr->front() == username){
				if ((*itr).size() == 2) {
					friendNames.clear();
					return friendNames;
				}
				else{
					auto itr_temp = (*itr).begin();
					itr_temp++;
					itr_temp++;
					for (auto user_itr = itr_temp;
							user_itr != (*itr).end(); user_itr++){
						friendNames.push_back(*user_itr);
					}
					return friendNames;
				}
			}
	}
	friendNames.clear();
	return friendNames;		//username does not exist
}

std::string UserInfo::getFriendsStr(std::string & username){
	std::vector<std::string> v = getFriends(username);
	std::string str;
	if (v.size() == 0) return str;
	for(auto i = 0; i < v.size(); i++){
		str += v[i];
		str += ' ';
	}
	if (str.size() > 0) str.pop_back();
	return str;
}

std::list<std::string> UserInfo::parseToken(std::string & line){
	std::list<std::string> li;
	std::string str;

	for (int i = 0; i < line.size(); i++){
		if (line[i] == ' '){
			if (str.size() != 0){
				li.push_back(str);
				str.clear();
			}
		}
		else if (line[i] != '\0'){
			str.push_back(line[i]);
		}
	}
	li.push_back(str);
	return li;
}



