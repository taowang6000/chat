/*
 * FriendInfo.h
 *
 *  Created on: Nov 8, 2016
 *      Author: tao
 */
// offline, online and connected friends' list, potential friends' info
#ifndef FRIENDINFO_H_
#define FRIENDINFO_H_
#include <list>
#include <string>
#include <vector>


class FriendInfo{
public:
	FriendInfo();
	~FriendInfo();

	//add offline friend, return false if friend already exists, otherwise
	//insert the friend and return true
	bool addFriend(std::string & name);
	//remove offline friend, return false if friend is online, connected or not
	//exists; otherwise remove the friend and return ture
	bool removeFriend(std::string & name);

	//get the names of online friends, connected friends
	std::vector<std::string> getOnlineFriends();
	std::vector<std::string> getConnectedFriends();

	//judge if there is the friend, if the friend is online, and if the
	//friend is connected respectively
	bool find(std::string & name);
	bool isOnline(std::string & name);
	bool isConnected(std::string & name);
	bool isMessaged(std::string & name);


	bool isSent(std::string & name);		//if the name is in "invitation_sent"
	bool isReceived(std::string & name); //if the name is in "invitation_received"
	bool addSent(std::string & name);	//add the name into "invitation_sent"
	bool addReceived(std::string & name);//add the name into "invitation_received"
	bool removeSent(std::string & name);
	bool removeReceived(std::string & name);

	//return false if friend not found, otherwise set online, add IPaddr and
	//port, then return true
	bool setOnline(std::string & name, std::string & IPaddr, std::string & port);
	//return false if friend not found, otherwise set offline  and return true
	bool setOffline(std::string & name);	//400 received
	//once connected, the friend will keep connected status until process ends
	//or the other side disconnected
	bool setConnected(std::string & name, int sd);
	//set the friend as disconnected by socket number
	bool setDisconnected(int sd);
	//set the friend as having offline message
	bool setMessageOn(std::string &name);
	bool setMessageOff(std::string &name);

	int getSockByName(std::string & name);
	std::string getNameBySock(int sd);
	std::string getIPaddr(std::string & name);
	std::string getPort(std::string & name);

	//print all friends, online friends, and connected friends respectively
	void dump();
	void clear();

	size_t currentSize(){
		return friendList.size();
	}

private:
	struct Node{
		bool online;
		bool connected;
		bool message;		//whether this friend has offline messages
		std::string friendName;
		std::string IPaddress;
		std::string port;
		int sd;
	};
	std::list<Node> friendList;
	std::list<std::string> invi_sent;	 //list of users to whom invitation has been sent
	std::list<std::string> invi_received;//list of users from whom invitation has been received
};






#endif /* FRIENDINFO_H_ */
