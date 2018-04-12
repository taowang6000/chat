/*
 * Client.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: tao
 */
#include <sys/socket.h>
#include <sys/wait.h>
#include <strings.h>
#include <string.h>						//memset, sizeof
#include <unistd.h>						//wirte,read, close
#include <netinet/in.h>
#include <arpa/inet.h>					//inet_ntoa
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>						//AI_PASSIVE, getaddrinfo
#include <signal.h>						//sigaction
#include <pthread.h>

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
//#include <utility>
#include "FriendInfo.h"
#include "ClientProt.h"
#include "ClientConfig.h"


const std::string PORT = "5100";
const unsigned MAXBUFLEN = 1024;
const unsigned FILE_PIECE_SIZE = 1000;
int sent_counter = 0;
int received_counter = 0;
int irregular_counter = 0;		// if file piece length is not 1000

ClientConfig cc;
std::string myname;
std::string myIPaddr;
std::string myPort;			//port used to connecting with ther server
bool registered = false;
bool register_err = false;
bool login_err = false;

//receiving side agree to receive this file or not, 0: not known;
//1: ready to receive 2:reject to receive
int receiver_side_status = 0;
pthread_mutex_t rs_lock=PTHREAD_MUTEX_INITIALIZER;	//protect "receiver_side_staus"
bool effective_file_header = false;	// a effective file header received
pthread_mutex_t fh_lock=PTHREAD_MUTEX_INITIALIZER;	//protect the file header bit
int sockfd_for_server;		//socket number used for communicating with server
pthread_mutex_t ss_lock=PTHREAD_MUTEX_INITIALIZER;	//protect the socket writing to sever
bool logged_in = false;		// whether the client is logged in or not
pthread_mutex_t log_lock=PTHREAD_MUTEX_INITIALIZER;	//protect "logged_in"
FriendInfo fi;
pthread_mutex_t fi_lock=PTHREAD_MUTEX_INITIALIZER;	//protect FriendInfo
std::list<int> sockfds;			//list of active sockets
pthread_mutex_t sd_lock=PTHREAD_MUTEX_INITIALIZER;	//protect sockfds
FILE * fp_global;
pthread_mutex_t fp_lock=PTHREAD_MUTEX_INITIALIZER;	//protect pointer "fp_global" and the content it pointed

pid_t pid_call;
pid_t pid_listen;
bool caller = false;

void Menu()
{
  std::cout << "\n\n";
  std::cout << "r - register with the server" << std::endl;
  std::cout << "l - log into the server" << std::endl;
  std::cout << "exit - Exit program" << std::endl;
  std::cout << "\nEnter choice : ";
}

void subMenu(){
	std::cout << "\n\n";
	std::cout << "m friend_name [message_body] 	- send a message to a  friend" << std::endl;
	std::cout << "i user_name [message_body]  	- invite a friend" << std::endl;
	std::cout << "ia inviter_name [message_body]	- accept a invitation" << std::endl;
	std::cout << "o friend_name [message body]	- send a offline message" << std::endl;
	std::cout << "g friend_name    		- get friend's offline messages" << std::endl;
	std::cout << "f friend_name file_name		- send a file to connected friend" << std::endl;
	std::cout << "s				- show friends' status" << std::endl;
	std::cout << "v friend_name 		- make a voice call with a friend" << std::endl;
	std::cout << "d				-disconnect the voice call with the friend" << std::endl;
	std::cout << "logout				- logout the server" <<std::endl;
	std::cout << "\nEnter choice : ";
}

std::vector<std::string> parseToken(std::string & line){
	std::vector<std::string> v;
	std::string str;

	for (int i = 0; i < line.size(); i++){
		if (line[i] == ' '){
			if (str.size() != 0){
				v.push_back(str);
				str.clear();
			}
		}
		else if (line[i] != '\0'){
			str.push_back(line[i]);
		}
	}
	v.push_back(str);
	return v;
}

/*
 * handler for signal INT
 * close open socket, save user information and exit
 */
static void process_signal(int signo){
	if (signo == SIGINT){
		for (auto itr = sockfds.begin(); itr != sockfds.end(); itr++){
			close(*itr);
		}
		while(waitpid(-1, NULL, WNOHANG) > 0);	//reap all possible dead child process
		if (caller){
			kill(pid_call, SIGKILL);
			caller = false;
		}
		kill(pid_listen, SIGKILL);
		exit(0);
	}
	else
		std::cout << "unexpected signal received: " << signo << std::endl;
}

void processMessage(int fd, char * buf, int n){
	ClientProt cp;
	int type;

	cp.readBuffer(buf,n);
	type = cp.getType();
	switch (type) {
	case 100:{				// add a friend's location
		/*update FriendInfo */
		std::cout << "100 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		pthread_mutex_lock(&fi_lock);
		fi.setOnline(v[0],v[1],v[2]);
		pthread_mutex_unlock(&fi_lock);
		std::cout << "----------------Current friend status:----------------"
				<< std::endl;
		fi.dump();
		break;
	}
	case 200:{				//login ACK
		/*set logged_in boolean value
		 *send my location information to server
		 */
		std::cout << "200 received" << std::endl;
		pthread_mutex_lock(&log_lock);
		logged_in = true;
		pthread_mutex_unlock(&log_lock);
		std::string payload;
		payload = myname + ' ' + myIPaddr + ' ' + myPort;
		cp.setType(301);
		cp.setLengthAndPayload(payload);
		cp.writeSock(fd);
		std::cout << "301 sent" << std::endl;
		//std::cout << "fd number: " << fd << std::endl;
		break;
	}
	case 300:{				//friends' locations
		/* update FriendInfo of friends*/
		std::cout << "300 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		for (int i = 0; i < v.size() / 3; i++) {
			pthread_mutex_lock(&fi_lock);
			fi.setOnline(v[3*i],v[3*i + 1],v[3*i + 2]);
			pthread_mutex_unlock(&fi_lock);
		}
		//std::cout << "----------------Current friend status:----------------"
						//<< std::endl;
		//fi.dump();
		break;
	}
	case 400:{				//a friend is offline
		/*update FriendInfo */
		std::cout << "400 received" << std::endl;
		pthread_mutex_lock(&fi_lock);
		fi.setOffline(cp.getPayload()[0]);
		pthread_mutex_unlock(&fi_lock);
		break;
	}
	case 500:{				//login NAK, register NAK
		std::cout << "500 received" << std::endl;
		std::cout << "Incorrect password, try again." << std::endl;
		login_err = true;
		break;
	}
	case 600:{				//invitation received
		/* update FriendInfo invi_received list
		 * display the message (if any)
		 */
		std::cout << "600 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		if (fi.isReceived(v[1])) break;
		pthread_mutex_lock(&fi_lock);
		fi.addReceived(v[1]);
		pthread_mutex_unlock(&fi_lock);
		std::cout << "There is an invitation from: " << v[1] <<std::endl;
		if (v.size() > 2){
			std::cout << v[1] << ": ";
			for (auto i = 2; i < v.size(); i++){
				std::cout << v[i] << ' ';
			}
			std::cout << std::endl;
		}
		break;
	}
	case 700:{				//invitation acceptance received
		/* update FriendInfo invi_sent list, update fristd::endlist,
		 * display message (if any)
		 */
		std::cout << "700 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		if (!fi.isSent(v[1])) break;
		pthread_mutex_lock(&fi_lock);
		fi.removeSent(v[1]);
		fi.addFriend(v[1]);
		pthread_mutex_unlock(&fi_lock);
		std::cout << "Friend invitation request accepted by " << v[1] <<std::endl;
		if (v.size() > 2){
			std::cout << v[1] <<": ";
				for (auto i = 2; i < v.size(); i++){
					std::cout << v[i] << ' ';
				}
			std::cout << std::endl;
		}
		break;
	}
	case 800:{				//register ACK
		/* set "registered" boolean value */
		std::cout << "800 received" << std::endl;
		registered = true;
		std::cout << "User registered successfully." << std::endl;
		break;
	}
	case 900: {				//register NAK
		std::cout << "900 received" << std::endl;
		std::cout << "User exists, try again." << std::endl;
		register_err = true;
		break;
	}
	case 1000:{				//my friend list received
		std::cout << "1000 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		if (v.size() > 0){
			pthread_mutex_lock(&fi_lock);
			for (auto i = 0; i < v.size(); i++){
				fi.addFriend(v[i]);
			}
			pthread_mutex_unlock(&fi_lock);
		}
		//std::cout << "\n---------------Current friend status:----------------"
					//<< std::endl;
		//fi.dump();
		break;
	}
	case 1100:{				//friend names received (who has offline messages)
		std::cout << "1100 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		if (v.size() > 0){
			std::cout << "The following friends have offline messages: ";
			pthread_mutex_lock(&fi_lock);
			for (auto i = 0; i < v.size(); i++){
				std::cout << v[i] << ' ';
				fi.setMessageOn(v[i]);
			}
			pthread_mutex_unlock(&fi_lock);
			std::cout << std::endl;
		}
		break;
	}
	case 1200:{				//offline message received
		std::cout << "1200 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		if (v.size() > 0){
			pthread_mutex_lock(&fi_lock);
			fi.setMessageOff(v[0]);
			pthread_mutex_unlock(&fi_lock);
			if (v.size() > 1){
				std::cout << v[0] << ": ";
				for (auto i = 1; i < v.size(); i++){
					std::cout << v[i] << " ";
				}
				std::cout << std::endl;
			}
		}
		break;
	}
	case 102:{
		/* if the friend is labeled as  connected, update friend list and display
		 * the message, else display the message */
		std::cout << "102 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		if (!fi.isConnected(v[0])){
			fi.setConnected(v[0], fd);
		}
		std::cout << "There is a message from: " << v[0] << std::endl;
		std::cout << v[0] << ": ";
		if (v.size() > 1) {
			for (auto i = 1; i < v.size(); i++){
				std::cout << v[i] << ' ';
			}
			std::cout << std::endl;
		}
		break;
	}
	case 202:{			//file transfer starting notice
		std::cout << "202 received" << std::endl;
		if (effective_file_header) {		//only one file can be received concurrently
			cp.clear();
			cp.setType(602);
			cp.writeSock(fd);
			std::cout << "602 sent" << std::endl;
			break;
		}
		std::vector<std::string> v;
		v = cp.getPayload();
		if (v.size() > 1){
			pthread_mutex_lock(&fp_lock);
			fp_global = fopen(v[1].c_str(),"w");
			pthread_mutex_unlock(&fp_lock);
			if (fp_global == NULL){
				std::cout << "Open file failed, tell sender change file name and try again.\n";
				break;
			}
			pthread_mutex_lock(&fh_lock);
			effective_file_header = true;
			pthread_mutex_unlock(&fh_lock);
			cp.clear();
			cp.setType(502);
			cp.writeSock(fd);
			std::cout << "502 sent" << std::endl;
			irregular_counter = 0;
			received_counter = 0;
		}
		break;
	}
	case 302:{			//a piece of file block received
		std::cout << "302 received" << std::endl;
		if (effective_file_header == false) break;
		std::cout << cp.getLength() << " of bytes received.\n";
		pthread_mutex_lock(&fp_lock);
		int write_length = fwrite(cp.getRawPayload().c_str(), sizeof(char),
				cp.getLength(), fp_global);
		pthread_mutex_unlock(&fp_lock);
		std::cout << write_length << " of bytes written.\n";
		received_counter +=1;
		if (write_length != FILE_PIECE_SIZE){
			irregular_counter += 1;
		}
		break;
	}
	case 402:{			//file transfer ending notice
		std::cout << "402 received" << std::endl;
		std::vector<std::string> v;
		v = cp.getPayload();
		std::cout << "File " << v[1] << " finished receiving.\n";
		pthread_mutex_lock(&fh_lock);
		effective_file_header = false;
		pthread_mutex_unlock(&fh_lock);
		pthread_mutex_lock(&fp_lock);
		fclose(fp_global);
		pthread_mutex_unlock(&fp_lock);
		std::cout << "Total pieces received: " << received_counter << std::endl;
		std::cout << "Total irregular pieces : " << irregular_counter << std::endl;
		received_counter = 0;
		irregular_counter = 0;
		subMenu();
		break;
	}
	case 502:{			//receiver side agree to receive
		std::cout << "502 received" << std::endl;
		pthread_mutex_lock(&fi_lock);
		receiver_side_status = 1;
		pthread_mutex_unlock(&fi_lock);
		break;
	}
	case 602:{			//receiver side reject to receive
		std::cout << "602 received" << std::endl;
		pthread_mutex_lock(&fi_lock);
		receiver_side_status = 1;
		pthread_mutex_unlock(&fi_lock);
		break;
	}
	default:{
		break;
	}
	}	//switch
}

void *process_connection(void *arg) {
    int sockfd;
    ssize_t n;
    char buf[MAXBUFLEN];

    sockfd = *((int *)arg);
    free(arg);

    pthread_detach(pthread_self());
    while ((n = read(sockfd, buf, MAXBUFLEN)) > 0){
    	processMessage(sockfd, buf,n);
    }
    if (n <= 0) {
    	if (n == 0)		//another side closed the socket
    		std::cout << "connection closed" << std::endl;
    	else
    		std::cout << "something wrong when reading" << std::endl;
    	close(sockfd);
    	pthread_mutex_lock(&sd_lock);
    	sockfds.remove(sockfd);
    	pthread_mutex_unlock(&sd_lock);
    	//the other side is server, exit with error code
    	if (sockfd_for_server == sockfd){
    		std::cout << "server side disconnected. " << std::endl;
    	}
    	//the other side is client, change the status into disconnected
    	else{
    		pthread_mutex_lock(&fi_lock);
    		fi.setDisconnected(sockfd);
    		pthread_mutex_unlock(&fi_lock);
    	}
    }
    return NULL;
}


void process_submenu(){
	ClientProt cp, cp_sub;
	std::string str, wholeline, token;
	std::vector<std::string> v;
	subMenu();
	while (getline(std::cin,wholeline)){
		if (logged_in){
			v= parseToken(wholeline);
			if (v.size() <2 ) {
				if (v[0].compare("logout") != 0 && v[0].compare("s") != 0
						&& v[0].compare("d") != 0){
					std::cout<< "Incorrect format "  << std::endl;
					continue;
				}
			}

			if(v[0].compare("m") == 0){		//send a message
				/* if the friend is not connected, initiate a connection ,
				 * update friend list status and send the message; else
				 * get the connected friend's socket number, send the message*/
				if (!fi.find(v[1])){
					std::cout<< "Friend " << v[1] << " does not exist." << std::endl;
				}
				else if (!fi.isOnline(v[1])){	//friend is off-line
					std::cout<< "Friend " << v[1] << " is offline." << std::endl;
				}
				else if (!fi.isConnected(v[1])){ //establish a new connection
					struct addrinfo hints, *ai_client, *p_client;
					int rv, flag, sd, *sock_ptr;;
					pthread_t tid;

					memset(&hints, 0, sizeof(struct addrinfo));	// make sure the struct is empty

					hints.ai_family = AF_UNSPEC;		// don't care IPv4 or IPv6
					hints.ai_socktype = SOCK_STREAM;	// TCP or SCTP stream sockets
					hints.ai_protocol = IPPROTO_TCP;	// TCP only
					std::cout << "IP address: " << fi.getIPaddr(v[1]) << std::endl;
					if ((rv = getaddrinfo(fi.getIPaddr(v[1]).c_str(),PORT.c_str(),
							&hints, &ai_client)) != 0) {
						std::cout << "getaddrinfo wrong: " << gai_strerror(rv) << std::endl;
						exit(2);
					}
					flag = 0;
					for(p_client = ai_client; p_client != NULL; p_client = p_client->ai_next){
						sd = socket(p_client->ai_family, p_client->ai_socktype, p_client->ai_protocol);
						if (sd < 0)
							continue;
						if (connect(sd, p_client->ai_addr, p_client->ai_addrlen) == 0) {
							flag = 1;
							break;
						}
						close(sd);
					}

					freeaddrinfo(ai_client);
					//freeaddrinfo(p);
					if (flag == 0) {
						std::cout << "cannot connect the other client" << std::endl;
						exit(3);
					}

					pthread_mutex_lock(&sd_lock);
					sockfds.push_back(sd);
					pthread_mutex_unlock(&sd_lock);
					sock_ptr = (int *)malloc(sizeof(int));
					*sock_ptr = sd;

					pthread_create(&tid, NULL, &process_connection, (void *)sock_ptr);
					//sleep(1);
					fi.setConnected(v[1],sd);
					cp.clear();
					cp.setType(102);
					std::string payload = myname;
					if (v.size() > 2){
						for (auto i = 2; i < v.size(); i++) {
							payload = payload + ' ' + v[i];
						}
					}
					cp.setLengthAndPayload(payload);
					cp.writeSock(sd);
					std::cout << "102 sent" << std::endl;

				}
				else{
					cp.clear();
					cp.setType(102);
					std::string payload = myname;
					if (v.size() > 2){
						for (auto i = 2; i < v.size(); i++) {
							payload = payload + ' ' + v[i];
						}
					}
					cp.setLengthAndPayload(payload);
					cp.writeSock(fi.getSockByName(v[1]));
					std::cout << "102 sent" << std::endl;
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("i") == 0){	//invite a new friend
				/*send the friend invitation request to the server
				 * update friend_sent list*/
				if (fi.find(v[1])){
					std::cout << v[1] << " is already my friend." << std::endl;
				}
				else if (fi.isSent(v[1])){
					std::cout << "invitation of " << v[1] << "has already sent" << std::endl;
				}
				else{
					cp.clear();
					cp.setType(601);
					std::string payload;
					payload = v[1] + ' ' + myname;
					if (v.size() > 2) {
						for (auto i = 2; i < v.size(); i++){
							payload += ' ';
							payload += v[i];
						}
					}
					cp.setLengthAndPayload(payload);
					pthread_mutex_lock(&ss_lock);
					cp.writeSock(sockfd_for_server);
					pthread_mutex_unlock(&ss_lock);
					pthread_mutex_lock(&fi_lock);
					fi.addSent(v[1]);
					pthread_mutex_unlock(&fi_lock);
					std::cout << "601 sent" << std::endl;
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("ia") == 0){		//invitation acceptance
				/*send invitation acceptance to friend, update FriendInfo*/
				if (!fi.isReceived(v[1])){
					std::cout << "invitation from " << v[1] <<
							" has not been received yet" << std::endl;
				}
				else{
					cp.clear();
					cp.setType(701);
					std::string payload;
					payload = v[1] + ' ' + myname;
					if (v.size() > 2) {
						for (auto i = 2; i < v.size(); i++){
							payload += ' ';
							payload += v[i];
						}
					}
					cp.setLengthAndPayload(payload);
					pthread_mutex_lock(&ss_lock);
					cp.writeSock(sockfd_for_server);
					pthread_mutex_unlock(&ss_lock);
					std::cout << "701 sent" << std::endl;
					pthread_mutex_lock(&fi_lock);
					fi.removeReceived(v[1]);
					fi.addFriend(v[1]);
					pthread_mutex_unlock(&fi_lock);
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("o") == 0){		//send offline messages to friend
				if (!fi.find(v[1])){
					std::cout<< "Friend " << v[1] << " does not exist." << std::endl;
				}
				else if (fi.isOnline(v[1])){	//friend is off-line
					std::cout<< "Friend " << v[1] << " is online." << std::endl;
				}
				else {
					cp.clear();
					cp.setType(901);
					std::string payload;
					payload = v[1] + ' ' + myname + ' ';
					if (v.size() > 2) {
						for (auto i = 2; i < v.size(); i++){
							payload = payload + v[i] + ' ';
						}
					}
					payload.pop_back();
					cp.setLengthAndPayload(payload);
					pthread_mutex_lock(&ss_lock);
					cp.writeSock(sockfd_for_server);
					pthread_mutex_unlock(&ss_lock);
					std::cout << "901 sent" << std::endl;
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("s") == 0){		//display all the friends' status
				std::cout << "----------------Current friend status:----------------\n";
				fi.dump();
				v.clear();
				subMenu();
			}
			else if (v[0].compare("f") == 0){		//send a file to a online friend
				if (!fi.find(v[1])){
					std::cout<< "Friend " << v[1] << " does not exist." << std::endl;
				}
				else if (!fi.isOnline(v[1])){		//friend is off-line
					std::cout<< "Friend " << v[1] << " is offline." << std::endl;
				}
				else if (!fi.isConnected(v[1])){
					std::cout<< "Friend " << v[1] << " is not connected,"
							"please send a message to get connected"<< std::endl;
				}
				else{
					FILE *fp = fopen(v[2].c_str(), "r");
					if (fp == NULL){
						std::cout << "File not found.\n";
						v.clear();
						subMenu();
						continue;
					}
					cp.clear();						//send starting notice
					cp.setType(202);
					std::string payload = myname + ' ' + v[2];
					cp.setLengthAndPayload(payload);
					cp.writeSock(fi.getSockByName(v[1]));
					std::cout << "202 sent\n";

					while(receiver_side_status != 1){
						if (receiver_side_status == 2){
							std::cout << "receiving side busy, try again after a moment.\n";
							pthread_mutex_lock(&rs_lock);
							receiver_side_status = 0;
							pthread_mutex_unlock(&rs_lock);
							v.clear();
							subMenu();
							continue;
						}
					}

					int file_piece_length = 0;		//send file body
					char file_buf[FILE_PIECE_SIZE];
					bzero(file_buf, sizeof(file_buf));
					sent_counter = 0;
					while ((file_piece_length = fread(file_buf, sizeof(char),
							FILE_PIECE_SIZE, fp)) > 0){
						std::cout << "File piece length = " << file_piece_length
								<< std::endl;
						cp.clear();
						cp.setType(302);
						cp.setLengthAndPayload(file_buf);
						cp.setLength(file_piece_length);
						cp.writeSock(fi.getSockByName(v[1]));
						bzero(file_buf, sizeof(file_buf));
						std::cout << "302 sent\n";
						sent_counter += 1;
						usleep(20000);
					}
					fclose(fp);
					std::cout << "Total pieces sent: " << sent_counter << std::endl;
					cp.clear();						//send ending notice
					cp.setType(402);
					payload.clear();
					payload = myname + ' ' + v[2];
					cp.setLengthAndPayload(payload);
					cp.writeSock(fi.getSockByName(v[1]));
					std::cout << "402 sent\n";
					pthread_mutex_lock(&rs_lock);
					receiver_side_status = 0;
					pthread_mutex_unlock(&rs_lock);
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("g") == 0){		//get a friend's offline messages
				if (!fi.isMessaged(v[1])){
					std::cout<< "Friend " << v[1] << " does not have offline message."
							<< std::endl;
				}
				else {
					std::string payload;
					payload = myname + ' ' + v[1];
					cp.clear();
					cp.setType(801);
					cp.setLengthAndPayload(payload);
					pthread_mutex_lock(&ss_lock);
					cp.writeSock(sockfd_for_server);
					pthread_mutex_unlock(&ss_lock);
					std::cout << "801 sent" << std::endl;
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("v") == 0){// make a voice call with a friend
				if (!fi.find(v[1])){
					std::cout<< "Friend " << v[1] << " does not exist." << std::endl;
				}
				else if (!fi.isOnline(v[1])){		//friend is off-line
					std::cout<< "Friend " << v[1] << " is offline." << std::endl;
				}
				else if (!fi.isConnected(v[1])){
					std::cout<< "Friend " << v[1] << " is not connected,"
							"please send a message to get connected"<< std::endl;
				}
				else{
					//int status;
					char *_argv_call[4];
					pid_call = fork();
					_argv_call[0] = (char *)"./voice\0"; // voice program
					_argv_call[1] = (char *)"-c\0";  // connect
					_argv_call[2] = (char *)fi.getIPaddr(v[1]).c_str();
					_argv_call[2] = strcat(_argv_call[2], ":9100\0");

					//_argv_call[2] = "172.27.50.116:9100\0";  // connect the user with IP and Port
					_argv_call[3] = NULL;

					if (pid_call == 0) {
						if ((setpgid(0,0))< 0){		//set the child process to background
							std::cout << "setpgid error" << std::endl;
							exit(1);
						}
						if (execvp(_argv_call[0], _argv_call) == -1) {
					         perror("myvoice");
						}
					     exit(EXIT_FAILURE);
					} else if (pid_call < 0) {
					    perror("myvoice");
					}
					if ((setpgid(pid_call,pid_call))< 0){		//set the child process to background
						std::cout << "setpgid error" << std::endl;
						exit(1);
					}
					caller = true;
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("d") == 0){// disconect the call, send a signal
				if (caller){
					kill(pid_call, SIGKILL);
					caller = false;
				}
				v.clear();
				subMenu();
			}
			else if (v[0].compare("logout") == 0){
				cp.clear();
				cp.setType(401);
				pthread_mutex_lock(&ss_lock);
				cp.writeSock(sockfd_for_server);
				pthread_mutex_unlock(&ss_lock);
				std::cout << "401 sent" << std::endl;
				pthread_mutex_lock(&log_lock);
				logged_in = false;
				pthread_mutex_unlock(&log_lock);
				if (caller){
					kill(pid_call, SIGKILL);
					caller = false;
				}

				return;
			}
		}
		else{
			std::cout << "Incorrect input, try again." << std::endl;
			subMenu();
		}
	}
}

void * process_menuinput(void *arg){
	ClientProt cp;
	std::string str, str_username, str_passwd, payload;
	Menu();
	while (getline(std::cin,str)){
		if (str.c_str()[0] == 'r' || str.c_str()[0] == 'l'){
			str_username.clear();
			str_passwd.clear();
			std::cout << "Enter username: ";
			getline(std::cin,str_username);
			myname = str_username;
			std::cout << "Enter password: ";
			getline(std::cin,str_passwd);
			std::cout << std::endl;
			payload.clear();
			payload = str_username + ' ';
			payload += str_passwd;
			if (str.size() == 0) continue;
			if (str.c_str()[0] == 'r'){
				/* send register request to the server*/
				if (!registered){
					cp.clear();
					cp.setType(101);
					cp.setLengthAndPayload(payload);
					pthread_mutex_lock(&ss_lock);
					cp.writeSock(sockfd_for_server);
					pthread_mutex_unlock(&ss_lock);
					std::cout << "101 sent"  << std::endl;
					for ( ; ; ){
						if (register_err){
							register_err = false;
							break;
						}
						if (registered){
							//myname = str_username;
							break;
						}
					}
				}
				else {
					std::cout << "Already registered as " << myname << std::endl;
				}
				Menu();
			}
			else{			//str.c_str()[0] == 'l'
				/* send login request to the server*/
				if (!logged_in){
					cp.clear();
					cp.setType(201);
					cp.setLengthAndPayload(payload);
					pthread_mutex_lock(&ss_lock);
					cp.writeSock(sockfd_for_server);
					pthread_mutex_unlock(&ss_lock);
					std::cout << "201 sent"  << std::endl;
					for ( ; ; ){
						if (login_err){
							login_err = false;
							break;
						}
						if (logged_in){
							//myname = str_username;
							process_submenu();
							break;
						}
					}
				}
				else{
					std::cout << "Already logged in as " << myname << std::endl;
				}
				Menu();
			}
		}
		else if (str == "exit"){
			for (auto itr = sockfds.begin(); itr != sockfds.end(); itr++){
				close(*itr);
			}
			if (caller){
				kill(pid_call, SIGKILL);
				caller = false;
			}
			kill(pid_listen, SIGKILL);
			exit(0);
		}
		else {
			std::cout << "Incorrect input format, try again."  << std::endl;
			Menu();
		}
	}
	return NULL;
}


int main(int argc, char *argv[]){
	if (argc < 2) {
		std::cout<<"usage: "<<argv[0]<<" configuration_file "<<std::endl;
		exit(1);
	}
	/* -----------------set signal processing routine------------------------*/
	struct sigaction act;
	act.sa_handler = process_signal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGINT, &act, NULL) < 0)
		std::cout << "can not catch signal SIGINT" << std::endl;

	/* -------------------load configuration file----------------------------*/
	if (!cc.load(argv[1])){
			std::cout << "Error: Cannot open file " << argv[2] << std::endl;
	}

	/* ------------------start income call waiting process ------------------*/

	//int status;
	char *_argv[6];
	pid_listen = fork();
	_argv[0] = (char *)"./voice\0"; // voice program
	_argv[1] = (char *)"-a\0";  // accept the call automatically
	_argv[2] = (char *)"-l\0";  // listen command
	_argv[3] = (char *)"-p\0";  // command for the port
	_argv[4] = (char *)"9100"; //give the port number --- can change it to another valid port
	_argv[5] = NULL;

	if (pid_listen == 0) {
		if ((setpgid(0,0))< 0){		//set the child process to background
			std::cout << "setpgid error" << std::endl;
			exit(1);
		}
		if (execvp(_argv[0], _argv) == -1) {
	         perror("myvoice");
		}
	     exit(EXIT_FAILURE);
	} else if (pid_listen < 0) {
	    perror("myvoice");
	}
	if ((setpgid(pid_listen,pid_listen))< 0){		//set the child process to background
		std::cout << "setpgid error" << std::endl;
		exit(1);
	}

	/*--------behaving as a client, connect the server-----------------------*/
	struct addrinfo hints, *ai, *p;
	int rv, flag, sd, *sock_ptr;;
	pthread_t tid;

	memset(&hints, 0, sizeof(struct addrinfo));	// make sure the struct is empty

	hints.ai_family = AF_UNSPEC;		// don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP or SCTP stream sockets
	hints.ai_protocol = IPPROTO_TCP;	// TCP only
	if (!cc.find("servhost")){
		std::cout << "there is no servhost item in configuration file." << std::endl;
		exit(1);
	}
	if (!cc.find("servport")){
		std::cout << "there is no servport item in configuration file." << std::endl;
		exit(1);
	}
	if ((rv = getaddrinfo(cc.getvalue("servhost").c_str(),
			cc.getvalue("servport").c_str(), &hints, &ai)) != 0) {
		std::cout << "getaddrinfo wrong: " << gai_strerror(rv) << std::endl;
		exit(2);
	}
	flag = 0;
	for(p = ai; p != NULL; p = p->ai_next){
		sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sd < 0)
			   continue;
		if (connect(sd, p->ai_addr, p->ai_addrlen) == 0) {
			   flag = 1;
			   break;
		}
		close(sd);
	}
	struct sockaddr_in local_addr;
	socklen_t sock_len;
	sock_len = sizeof(local_addr);
	getsockname(sd, (struct sockaddr *)&local_addr, &sock_len);
	myIPaddr = inet_ntoa(local_addr.sin_addr);
	int i = ntohs(local_addr.sin_port);
	std::stringstream ss;
	ss << i;
	ss >> myPort;
	std::cout << "Local IP address is:  " << inet_ntoa(local_addr.sin_addr) <<
			"local port is: " << ntohs(local_addr.sin_port)
			<< std::endl;

	if (flag == 0) {
		std::cout << "cannot connect server" << std::endl;
		exit(3);
	}

	sockfd_for_server = sd;
	pthread_mutex_lock(&sd_lock);
	sockfds.push_back(sd);
	pthread_mutex_unlock(&sd_lock);
	sock_ptr = (int *)malloc(sizeof(int));
	*sock_ptr = sd;

	pthread_create(&tid, NULL, &process_connection, (void *)sock_ptr);

	/*-----create a thread to process the menus and input--------------------*/
	pthread_create(&tid, NULL, &process_menuinput, NULL);

	/*---behaving as a server, waiting for other clients' connection---------*/
	for (; ;) {
		if (logged_in){
			int listener, i;
			memset(&hints, 0, sizeof(struct addrinfo));	// make sure the struct is empty

			hints.ai_family = AF_UNSPEC;		// don't care IPv4 or IPv6
			hints.ai_socktype = SOCK_STREAM;	// TCP or SCTP stream sockets
			hints.ai_flags = AI_PASSIVE;		// fill IP address for me
			hints.ai_protocol = IPPROTO_TCP;	// TCP only

			if ( (i = getaddrinfo(myIPaddr.c_str(), PORT.c_str(), &hints, &ai)) != 0) {
				std::cout << "behaving as server: " << gai_strerror(i) << std::endl;
				exit(4);
			}

			for(p = ai; p != NULL; p = p->ai_next) {
				listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
				if (listener < 0) {
					continue;
				}

				// avoid："address already in use"
				int yes = 1;
				setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

				if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
					close(listener);
					continue;
				}
				break;
			}
			if (p == NULL) {
				std::cout << "selectserver: failed to bind" << std::endl;
				exit(5);
			}
			//freeaddrinfo(ai);
			freeaddrinfo(p);

			if (listen(listener, 10) == -1) {
				std::cout <<"listen error" << std::endl;
				exit(6);
			}

			struct sockaddr_in remote_addr, local_addr;
			socklen_t sock_len;
			int newfd;
			for (; ;) {
				if (logged_in){
					sock_len = sizeof(remote_addr);
					newfd = accept(listener, (struct sockaddr *)&remote_addr, &sock_len);
					pthread_mutex_lock(&sd_lock);
					sockfds.push_back(newfd);
					pthread_mutex_unlock(&sd_lock);
					sock_ptr = (int *)malloc(sizeof(int));
					*sock_ptr = newfd;

					pthread_create(&tid, NULL, &process_connection, (void*)sock_ptr);

				}
				else{
					break;
				}
			}
		}

	}
}






