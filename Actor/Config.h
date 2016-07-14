/*
 * Config.h
 *
 *  Created on: Jul 12, 2016
 *      Author: lieven
 */

#ifndef ACTOR_CONFIG_H_
#define ACTOR_CONFIG_H_

#include <Str.h>
#include <Cbor.h>
#include <EEPROM.h>
#include <map>

class ConfigClass {
	Cbor* _cbor;
	std::map<const char*,Cbor*>* _map;
	void load();
	void save();

public:
	ConfigClass();
	virtual ~ConfigClass();
	void get(const char*, uint32_t &);
	void get(const char*, Str&);
	void set(const char*, uint32_t &);
	void set(const char*, Str&);
	void menu();
	void begin();
	void end();
};

ConfigClass Config;

#endif /* ACTOR_CONFIG_H_ */
