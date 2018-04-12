/*
 * ActiveUser.h
 *
 *  Created on: Oct 27, 2016
 *      Author: tao
 */

#ifndef ACTIVEUSER_H_
#define ACTIVEUSER_H_
#include <list>
#include <string>
#include <vector>

//using namespace std;

class ActiveUser {			//this class deals with active user information

public:
	ActiveUser();
	~ActiveUser();

	//check if user is in the activeLists
	bool contains(const std::string & name);

	//when 201 is received, insert user-name
	bool insert(const std::string & name);
	//when 301 is received, add all the other information of the user
	//return if the user does not exist, or the other information already exists
	bool update(const std::string & name,const std::string & IPaddr,
			const std::string & port, const int sd);

	//delete the user and the corresponding location if it is in the list. Return true
	//if user is deleted, return false otherwise (i.e., if name is not in the list).
	bool removeByName(const std::string & name);
	bool removeBySocket(const int sd);

	std::vector<std::string> getActiveUsers();
	// return location of the user "name"
	std::string getSingleLocation(std::string & name);
	//return locations of users in "names", if these names are in the activeLists
	std::string getLocations(std::vector<std::string> &names);
	// get socket numbers by names
	std::vector<int> getSocks(std::vector<std::string> &names);
	// get the name by the socket number
	std::string getNameBySock(int sd);
	// get the socket number by name
	int getSockByName(std::string & name);


	//delete all elements in the list
	void clear();

	//display all entries in the list.
	void dump();

	size_t currentSize(){
		return activeLists.size();
	}


private:
	struct Node{
		std::string username;
		std::string IPaddress;
		std::string portNumber;
		int socket;
	};
	//list of active user-names and locations information
	std::list<Node> activeLists;
};




#endif /* ACTIVEUSER_H_ */
