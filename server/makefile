Server.x: Server.o ActiveUser.o AppProt.o ConfigInfo.o UserInfo.o MessageInfo.o
	g++ -std=c++11 -o Server.x Server.o ActiveUser.o AppProt.o ConfigInfo.o UserInfo.o MessageInfo.o
Server.o: Server.cpp ActiveUser.h AppProt.h ConfigInfo.h UserInfo.h MessageInfo.h
	g++ -std=c++11 -c Server.cpp
ActiveUser.o: ActiveUser.cpp ActiveUser.h
	g++ -std=c++11 -c ActiveUser.cpp
AppProt.o: AppProt.cpp AppProt.h
	g++ -std=c++11 -c AppProt.cpp
ConfigInfo.o: ConfigInfo.cpp ConfigInfo.h
	g++ -std=c++11 -c ConfigInfo.cpp
UserInfo.o: UserInfo.cpp UserInfo.h
	g++ -std=c++11 -c UserInfo.cpp
MessageInfo.o: MessageInfo.h MessageInfo.cpp
	g++ -std=c++11 -c MessageInfo.cpp
clean:
	rm *.o Server.x
