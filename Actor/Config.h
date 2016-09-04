/*
 * Config.h
 *
 *  Created on: Jul 12, 2016
 *      Author: lieven
 */

#ifndef ACTOR_CONFIG_H_
#define ACTOR_CONFIG_H_

#include <Arduino.h>
#include <ArduinoJson.h>

class ConfigClass {
	void initialize();
	void initMagic();
	bool checkMagic();

public:
	ConfigClass();
	virtual ~ConfigClass();

	void load(String& config);
	void save(String& config);
	void load(JsonObject& config);
	void save(JsonObject& config);

	void get(const char*, uint32_t &, uint32_t defaultValue);
	void get(const char*, String&, const char* defaultValue);

	void set(const char*, uint32_t &);
	void set(const char*, String&);
};

extern ConfigClass Config;

#endif /* ACTOR_CONFIG_H_ */
