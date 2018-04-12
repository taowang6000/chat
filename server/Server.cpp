/*
 * Server.cpp
 *
 *  Created on: Oct 28, 2016
 *      Author: tao
 */
//#include <sys/select.h>
//#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <strings.h>
#include <string.h>				//memset
#include <unistd.h>						//wirte, read
//#include <netinet/in.h>
#include <arpa/inet.h>					//inet_ntoa
//#include <stdlib.h>
#include <netdb.h>						//AI_PASSIVE, getaddrinfo
#include <signal.h>						//sigaction

#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
//#include <utility>
#include "ActiveUser.h"
#include "UserInfo.h"
#include "ConfigInfo.h"
#include "AppProt.h"
#include "MessageInfo.h"

#define MAXBUFLEN 1024

//using namespace std;

/*
 * global variables
 */
UserInfo ui;
ActiveUser au;
ConfigInfo ci;
MessageInfo mi;
std::list<int> sockfds;
fd_set masters;
char * file_to_write;		//used for signal handler

/*
 * handler for signal INT
 * close open socket, save user information and exit
 */
static void process_signal(int signo){
	if (signo == SIGINT){
		for (auto itr = sockfds.begin(); itr != sockfds.end(); itr++)
			close(*itr);
		ui.write_to_file(file_to_write);
		exit(0);
	}
	else
		std::cout << "unexpected signal received: " << signo << std::endl;
}


void processMessage(int fd, char * buf, int n){
	AppProt ap;
	int type;

	ap.readBuffer(buf,n);
	type = ap.getType();

	switch (type) {
	case 101: {			// registration, user-name and password
		/* check if the user-name is available; update UserInfo std::lists*/
		std::cout << "101 received" << std::endl;
		std::vector<std::string> v;
		v = ap.getPayload();
		if (v.size() == 0) break;
		if (ui.find(ap.getPayload()[0])){ 	//user-name already exists
			ap.setType(900);
			ap.writeSock(fd);
		}
		else{								//add user-name and password
			ui.addUser(v[0],v[1]);
			std::cout << "------------Information of current users: ----------"
					<< std::endl;
			std::cout << "New user added." << std::endl;
			ui.dump();
			ap.setType(800);
			ap.writeSock(fd);
		}
		break;
	}
	case 201: {			//login, user-name and password
		/*check if user-name/password pair match, update ActiveUser std::list
		 * display a message showing the total number of users online
		 * send the user his friends' names
		 */
		std::cout << "201 received" << std::endl;
		std::vector<std::string> v;
		v = ap.getPayload();
		//std::cout << v[0] << " " << v[1] << std::endl;
		if (!ui.match(v[0],v[1])){//password incorrect
			ap.setType(500);
			ap.writeSock(fd);
		}
		else{								// update ActiveUser std::list
			au.insert(v[0]);
			ap.setType(200);
			ap.writeSock(fd);
			std::cout << "Total number of users online: " << au.currentSize() << std::endl;
			ap.clear();
			ap.setType(1000);
			ap.setLengthAndPayload(ui.getFriendsStr(v[0]));
			ap.writeSock(fd);
		}
		break;
	}
	case 301: {								// add client's location(IP,port)
		/* update ActiveUser std::list
		 * send the user his friends' locations
		 * send the user his friends' names who have offline messages
		 * send location to all friends
		 */
		std::cout << "301 received" << std::endl;
		std::vector<std::string> v,friend_names;
		v = ap.getPayload();
		friend_names = ui.getFriends(v[0]);

		//first, update ActiveUser std::list
		au.update(v[0],v[1],v[2],fd);
		std::cout << "Active user list updated." << std::endl;
		std::cout << "--------------New active user list: ----------------"
				<< std::endl;
		au.dump();

		//second, send the client his friends' locations using the format:
		//name1 IPaddr1 port1 name2 IPaddr2 port2 .......
		std::string locations = au.getLocations(friend_names);
		if (locations.size() != 0){
			ap.clear();
			ap.setType(300);
			ap.setLengthAndPayload(locations);
			ap.writeSock(fd);
			std::cout << "300 sent" << std::endl;
		}

		//third, send the user his friends' names who have offline messages
		std::string names=mi.getNames(v[0]);
		if (names.size() != 0){
			ap.clear();
			ap.setType(1100);
			ap.setLengthAndPayload(names);
			ap.writeSock(fd);
			std::cout << "1100 sent" << std::endl;
		}


		//last, send the client's location(format:name IPaddr port) to all his friends
		std::string location = au.getSingleLocation(v[0]);
		std::vector<int> sockets = au.getSocks(friend_names);
		//std::cout << "sockets size: " << sockets.size() << std::endl;
		if (sockets.size() != 0){
			ap.clear();
			ap.setType(100);
			ap.setLengthAndPayload(location);
			for (unsigned int i = 0; i < sockets.size(); i++)
				ap.writeSock(sockets[i]);
		}
		break;
	}
	case 401: {				//disconnect, without payload
		/*
		 * informs the friends that the client is offline;
		 * update the active user std::list(delete the client);
		 */
		std::cout << "401 received" << std::endl;
		//first, informs the friends
		std::string name = au.getNameBySock(fd);
		std::vector<std::string> friend_names = ui.getFriends(name);
		//std::cout << "friends size: " << friend_names.size() << std::endl;
		if (friend_names.size() != 0){
			std::vector<int> sockets = au.getSocks(friend_names);
			//std::cout << "sd size: " << sockets.size() << std::endl;
			if (sockets.size() != 0){
				ap.clear();
				ap.setType(400);
				ap.setLengthAndPayload(au.getNameBySock(fd));
				for (unsigned int i = 0; i < sockets.size(); i++) {
					ap.writeSock(sockets[i]);
					std::cout << "400 sent "  << std::endl;
				}
			}
		}

		//second, update the active user std::list
		au.removeBySocket(fd);
		std::cout << "active friend removed "  << std::endl;
		std::cout << "--------------New active user list: ----------------"
						<< std::endl;
		au.dump();
		break;
	}
	case 601: {				//invitation
		/* forward the message, do not touch the payload*/
		if (!ui.find(ap.getPayload()[0]) || !ui.find(ap.getPayload()[1])){
			std::cout << "User does not exist." << std::endl;
			break;
		}
		std::cout << "601 received" << std::endl;
		std::string rawPayload = ap.getRawPayload();
		std::string name_temp = ap.getPayload()[0];
		ap.clear();
		ap.setType(600);
		ap.setLengthAndPayload(rawPayload);
		ap.writeSock(au.getSockByName(name_temp));
		std::cout << "600 sent" << std::endl;
		std::cout << "forwarded content: " << rawPayload << std::endl;
		break;
	}
	case 701: {				//invitation acceptance
		/* forward the message
		 * update both users' friend std::list
		 * send each other's location information
		 */
		std::cout << "701 received" << std::endl;
		if (!ui.find(ap.getPayload()[0]) || !ui.find(ap.getPayload()[1])){
			std::cout << "User does not exist." << std::endl;
			break;
		}
		// first, forward the message
		std::string rawPayload = ap.getRawPayload();
		std::string name_dest = ap.getPayload()[0];
		std::string name_source = ap.getPayload()[1];
		ap.clear();
		ap.setType(700);
		ap.setLengthAndPayload(rawPayload);
		ap.writeSock(au.getSockByName(name_dest));
		std::cout << "700 sent" << std::endl;
		std::cout << "forwarded content: " << rawPayload << std::endl;

		// second, update both users' friend std::list, add as each other's friend
		ui.addFriend(name_dest, name_source);
		ui.addFriend(name_source, name_dest);
		//ui.dump();

		// third, send the invitor's location
		std::string loc_dest = au.getSingleLocation(name_dest);
		ap.clear();
		ap.setType(100);
		ap.setLengthAndPayload(loc_dest);
		ap.writeSock(fd);
		std::cout << "100 sent to the invitation acceptor" << std::endl;

		// fourth, send the acceptor's location
		int socket_temp = au.getSockByName(name_dest);
		std::string loc_source = au.getSingleLocation(name_source);
		ap.clear();
		ap.setType(100);
		ap.setLengthAndPayload(loc_source);
		ap.writeSock(socket_temp);
		std::cout << "100 sent to the inviter" << std::endl;
		break;
	}
	case 801:{				//requirement of get offline messages
		/* send all the messages under the user name */
		std::cout << "801 received" << std::endl;
		std::vector<std::string> v;
		std::list<std::string> listTemp;
		std::string payload;
		v = ap.getPayload();
		if (v.size() > 0){
			listTemp = mi.getMessages(v[0],v[1]);
			if (listTemp.size() > 0){
				for (auto itr = listTemp.begin(); itr != listTemp.end(); itr++){
					payload.clear();
					payload = v[1] + ' ' + *itr;
					ap.clear();
					ap.setType(1200);
					ap.setLengthAndPayload(payload);
					ap.writeSock(fd);
					std::cout << "1200 sent" << std::endl;
				}
			}
		}
		mi.removeMessages(v[0], v[1]);
		break;
	}
	case 901:{				//an offline message received
		/* add the offline message onto the messageList */
		std::cout << "901 received" << std::endl;
		std::vector<std::string> v;
		std::string message;
		v = ap.getPayload();
		if (v.size() > 2){
			for (int i = 2; i < v.size(); i++){
				message = message + v[i] + ' ';
			}
		}
		message.pop_back();
		mi.addMessage(v[0], v[1],message);
		mi.dump();
		break;
	}
	default: {
		std::cout << "Unexpected information." << std::endl;
		break;
	}
	}// switch
}

int main(int argc, char *argv[]){
	if (argc < 3){
		std::cout<<"usage: "<<argv[0]<<" user_info_file configuration_file "<<std::endl;
		exit(1);
	}
	/* -----------------set signal processing routine------------------------*/
	file_to_write = argv[1];
	struct sigaction act;
	act.sa_handler = process_signal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGINT, &act, NULL) < 0)
		std::cout << "can not catch signal SIGINT" << std::endl;

	/* -------------------load user information file-------------------------*/

	if (!ui.load(argv[1])){
			std::cout << "Error: Cannot open file " << argv[1] << std::endl;
	}
	//ui.dump();
	/* -------------------load configuration file----------------------------*/

	if (!ci.load(argv[2])){
		std::cout << "Error: Cannot open file " << argv[2] << std::endl;
	}

	/*-------------------get a socket, and bind it---------------------------*/
	struct addrinfo hints, *ai, *p;
	int listener,i;

	memset(&hints, 0, sizeof(struct addrinfo));	// make sure the struct is empty

	hints.ai_family = AF_UNSPEC;		// don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP or SCTP stream sockets
	hints.ai_flags = AI_PASSIVE;		// fill IP address for me
	hints.ai_protocol = IPPROTO_TCP;	// TCP only

	if (!ci.find("port")){
		std::cout << "there is no port item in configuration file." << std::endl;
		exit(1);
	}

	/* "NULL" option of getaddrinfo function, dynamiclly select any address of local host.
	 * Of course, you can directly write a designated IP address or a domain name instead
	 * of "NULL". */
	if ((i = getaddrinfo(NULL, ci.getvalue("port").c_str(), &hints, &ai)) != 0) {
	    std::cout << "selectserver: " << gai_strerror(i) << std::endl;
	    exit(2);
	  }

	for(p = ai; p != NULL; p = p->ai_next) {
	    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	    if (listener < 0) {
	    	continue;
	    }

	    // avoid："address already in use"
	    int yes = 1;
	    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	    if ((i = bind(listener, p->ai_addr, p->ai_addrlen)) < 0) {
	    	close(listener);
	    	continue;
	    }
	    break;
	}
	if (p == NULL) {
	    std::cout << "selectserver: failed to bind" << std::endl;
	    exit(3);
	  }
	  freeaddrinfo(ai);

	  /*-----------------display IP address and port number of the socket,
	   * especially useful when using wildcard IP and prot 0 parameter------*/
	  struct sockaddr_in remote_addr, local_addr;
	  socklen_t sock_len;

	  sock_len = sizeof(remote_addr);
	  getsockname(listener, (struct sockaddr *)&local_addr, &sock_len);
	  std::cout << "port = " << ntohs(local_addr.sin_port) << std::endl;

	  /*----------------------std::listen----------------------------------------*/
	  if (listen(listener, 10) == -1) {
	      std::cout <<"std::listen error" << std::endl;
	      exit(4);
	    }

	  /*----------------------main function----------------------------------*/
	  int newfd;
	  char buf[MAXBUFLEN];
	  fd_set readfds;
	  int maxfd,n;

	  FD_ZERO(&masters);
	  FD_ZERO(&readfds);
	  FD_SET(listener, &masters);

	  maxfd = listener;
	  sockfds.push_back(listener);

	  for ( ; ; ){
		  readfds = masters;

		  //time is set to NULL, wait forever, until there's something to read
		  select(maxfd+1, &readfds, NULL, NULL, NULL);

		  for (auto itr = sockfds.begin(); itr != sockfds.end(); itr++){
			  int sock_tmp = *itr;
			  if (FD_ISSET(sock_tmp, &readfds)){
				  if (sock_tmp == listener){	//new connection
					  //3 parameters return from kernal:newfd, client_addr,length
					  newfd = accept(listener, (struct sockaddr *)&remote_addr,
							  &sock_len);
					  std::cout << "New connection accepted." << std::endl;
					  std::cout<<"remote machine: "<<inet_ntoa(remote_addr.sin_addr)
						 <<" port: "<< ntohs(remote_addr.sin_port);
					  std::cout << std::endl;
					  std::cout << "socket number is: " << newfd << std::endl;
					  FD_SET(newfd, &masters);
					  if (newfd > maxfd)
					  			maxfd = newfd;
					  sockfds.push_back(newfd);
				  }
				  else{		//data received from client
					  n = read(sock_tmp, buf, MAXBUFLEN);
					  	  if (n <= 0) {
					  		  if (n == 0)		//another side closed the socket
					  			  std::cout << "connection closed" << std::endl;
					  		  else
					  			  std::cout << "something wrong when reading" << std::endl;
					  		  close(sock_tmp);
					  		  FD_CLR(sock_tmp, &masters);
					  		  sockfds.remove(sock_tmp);
					  		  /* 3 ways to delete a user from active user std::lists:
					  		   * logout or socket closed or disconnect on other side
					  		   */
					  		  au.removeBySocket(sock_tmp);	//update ActiveUser std::lists
					  		  itr--;	//to prevent itr increasing beyond sockfds.end()
					  	  }
					  	  else {
					  		  std::cout << "new data received" << std::endl;
					  		  processMessage(sock_tmp, buf, n);
					  		  //write(sock_tmp, buf, sizeof(buf));
					  	  }
				  }
			  }
		  }
	  }
}

