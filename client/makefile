Client.x: Client.o ClientConfig.o ClientProt.o FriendInfo.o
	g++ -std=c++11 -pthread -o Client.x Client.o ClientConfig.o ClientProt.o FriendInfo.o
Client.o: Client.cpp ClientConfig.h ClientProt.h FriendInfo.h
	g++ -std=c++11 -c Client.cpp
ClientConfig.o: ClientConfig.h ClientConfig.cpp
	g++ -std=c++11 -c ClientConfig.cpp
ClientProt.o: ClientProt.h ClientProt.cpp
	g++ -std=c++11 -c ClientProt.cpp
FriendInfo.o: FriendInfo.h FriendInfo.cpp
	g++ -std=c++11 -c FriendInfo.cpp
clean:
	rm *.o Client.x
