/*
 * Config.cpp
 *
 *  Created on: Jul 12, 2016
 *      Author: lieven
 */
#include <EEPROM.h>
#include "Config.h"

#include <Sys.h>
#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0xDEADBEEF

ConfigClass::ConfigClass() {

}

ConfigClass::~ConfigClass() {

}


void ConfigClass::initMagic() {
	uint32_t word = EEPROM_MAGIC;
	for (int i = 0; i < 4; i++) {
		EEPROM.write(i, (uint8_t) (word & 0xFF));
		word >>= 8;
	}
}

bool ConfigClass::checkMagic() {
	uint32_t word = 0;
	uint32_t i = 3;
	while (true) {
		word += EEPROM.read(i);
		if (i == 0)
			break;
		word <<= 8;
		i--;
	}
	if (word == EEPROM_MAGIC)
		return true;
	return false;
}

void ConfigClass::load(String& str) {
	EEPROM.begin(EEPROM_SIZE);
	if (!checkMagic()) {
		LOGF(" initialize EEPROM with empty config.");
		initMagic();
		int address = 4;
		EEPROM.write(address++, '{');
		EEPROM.write(address++, '}');
		EEPROM.write(address++, '\0');
	}
	int i = 4;
	uint8_t b;
	while (true) {
		b = EEPROM.read(i++);
		if (b == 0)
			break;
		str+=(char)b;
	}
	EEPROM.end();
//	LOGF(str.c_str());
}

void ConfigClass::save(String& str) {
//	LOGF(str.c_str());
	EEPROM.begin(EEPROM_SIZE);
	int address = 4;
	initMagic();
	for (int i = 0; i < str.length(); i++)
		EEPROM.write(address++, str.charAt(i));
	EEPROM.write(address++, 0);
	ASSERT_LOG(EEPROM.commit());
	EEPROM.end();

}

void ConfigClass::set(const char* key, String& value) {
	String input;
	load(input);
//	LOGF(" input :%s",input.c_str());
	StaticJsonBuffer<400> jsonConf;
	JsonObject& object = jsonConf.parseObject(input);
	object[key] = value;
	String output;
	object.printTo(output);
	save(output);
//	LOGF(" output :%s",output.c_str());

}

void ConfigClass::get(const char* key, String& value,
		const char* defaultValue) {
	String input;
	load(input);
//	LOGF(" input :%s",input.c_str());
	StaticJsonBuffer<400> jsonConf;
	JsonObject& object = jsonConf.parseObject(input);
	if (object.containsKey(key))
		value = (const char*) object[key];
	else {
		value = defaultValue;
		set(key,value);
	}
}

void ConfigClass::set(const char* key, uint32_t& value) {
	String input;
	load(input);
//	LOGF(" input :%s",input.c_str());
	StaticJsonBuffer<400> jsonConf;
	JsonObject& object = jsonConf.parseObject(input);
	object[key] = String(value);
	String output;
	object.printTo(output);
	save(output);
//	LOGF(" output :%s",output.c_str());

}

void ConfigClass::get(const char* key, uint32_t& value,
		uint32_t defaultValue) {
	String input;
	load(input);
//	LOGF(" input :%s",input.c_str());
	StaticJsonBuffer<400> jsonConf;
	JsonObject& object = jsonConf.parseObject(input);
	if (object.containsKey(key))
		value = object[key];
	else{
		value = defaultValue;
		set(key,value);
	}
}

ConfigClass Config;

