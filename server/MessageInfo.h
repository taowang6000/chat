/*
 * MessageInfo.h
 *
 *  Created on: Nov 21, 2016
 *      Author: tao
 */

#ifndef MESSAGEINFO_H_
#define MESSAGEINFO_H_
#include <list>
#include <string>
#include <vector>

class MessageInfo {		//this class deals with offline messages
public:
	MessageInfo();
	~MessageInfo();
	//update username, friendname and message information
	void addMessage(std::string & username, std::string & friendname, std::string & message);
	//remove all of messages of a user' friend, if the friend is the last friend(who have
	//messages), remove the user list
	bool removeMessages(std::string & username, std::string friendname);
	//get list of messages
	std::list<std::string> getMessages(std::string & username, std::string & friendname);

	bool find(std::string &name);
	void clear();
	void dump();
	//get names of the user's friends who have offline messages
	std::string getNames(std::string & username);

private:
	struct Node{
		std::string name;
		std::list<std::string> messages;
	};
	//every list in the lists corresponds to a user and its messages
	//format: userNode friend1_Node friend2_Node ...
	//only name domain exists in userNode; both domains exist in friend Node
	std::list<std::list<Node>> messageLists;
};




#endif /* MESSAGEINFO_H_ */
