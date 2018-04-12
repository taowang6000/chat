/*
 * ClientConfig.h
 *
 *  Created on: Nov 8, 2016
 *      Author: tao
 */

#ifndef CLIENTCONFIG_H_
#define CLIENTCONFIG_H_
#include <list>
#include <string>


class ClientConfig {				//this class deals with the configuration
public:
	ClientConfig();
	~ClientConfig();

	bool load(const char *filename);
	void clear();
	bool find(const std::string & name) const;
	std::string getvalue(const std::string & key) const;

private:
	std::list<std::pair<std::string,std::string>> configList;

	std::pair<std::string, std::string> parse(std::string & line);
};





#endif /* CLIENTCONFIG_H_ */
