/*
 * UserInfo.h
 *
 *  Created on: Oct 26, 2016
 *      Author: tao
 */

#ifndef USERINFO_H_
#define USERINFO_H_
#include <list>
#include <string>
#include <vector>


class UserInfo {		//this class deals with user account information

public:
	UserInfo();					// constructor
	~UserInfo();

	// Load a user information file into the userLists object
	bool load(const char *filename);

	// Add and remove a new user-name and password.  The password is in plain-text
	bool addUser(const std::string & name, const std::string password);
	bool removeUser(const std::string &name);

	// add friend; if user-name does not exist, return false; if friend already
	// existed, return false;
	bool addFriend(const std::string & username, const std::string & friendname);
	bool removeFriend(const std::string & username, const std::string & friendname);
	std::vector<std::string> getFriends(std::string & username);
	std::string getFriendsStr(std::string & username);

	/*
	bool changePassword(const std::string & name, const std::string & oldpassword,
			const std::string & newpassword);
	*/
	// Check if the name-password pair is in userLists
	bool match(const std::string & name, const std::string & password) const;

	// Check if a user exists
	bool find(const std::string & name) const;

	// Display all entries in userLists on the screen
	void dump();
	// //delete all elements in userLists
	void clear();

	bool write_to_file(const char *filename);


private:
	//all the user and his friends' information, and all user's offline message
	std::list<std::list<std::string>> userLists;

	//parse a line of "user_info_file" into list of string, "parseToken()"is called
	std::list<std::string> parse(std::string & line);
	//parse a line of words separated by ' ' into list of string
	std::list<std::string> parseToken(std::string & line);

};


#endif /* USERINFO_H_ */
