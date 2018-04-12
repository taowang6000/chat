/*
 * ConfigInfo.h
 *
 *  Created on: Oct 28, 2016
 *      Author: tao
 */

#ifndef CONFIGINFO_H_
#define CONFIGINFO_H_
#include <list>
#include <string>

//using namespace std;

class ConfigInfo {				//this class deals with the configuration
public:
	ConfigInfo();
	~ConfigInfo();

	bool load(const char *filename);
	void clear();
	bool find(const std::string & name) const;
	std::string getvalue(const std::string & key) const;

private:
	std::list<std::pair<std::string,std::string>> configList;

	std::pair<std::string, std::string> parse(std::string & line);
};

#endif /* CONFIGINFO_H_ */
