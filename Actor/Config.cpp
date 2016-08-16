/*
 * Config.cpp
 *
 *  Created on: Jul 12, 2016
 *      Author: lieven
 */

#include "Config.h"
#define SIZE 512
#define MAGIC 0xA1B1



ConfigClass::ConfigClass() {

}

ConfigClass::~ConfigClass() {

}

void ConfigClass::begin() {
	EEPROM.begin(SIZE);
	_cbor = new Cbor(SIZE);
	load();
}

void ConfigClass::load() {
	uint16_t magic, length;

	magic = EEPROM.read(0) + 256 * EEPROM.read(1);
	if (magic != 0xA1B1)
		return;
	length = EEPROM.read(2) + 256 * EEPROM.read(3);
	for (int i = 4; i < SIZE && i < length; i++) {
		_cbor->write(EEPROM.read(i));
	}
	_cbor->offset(0);
}

void ConfigClass::save() {
	uint16_t magic, length;

	EEPROM.write(0, MAGIC & 0xFF);
	EEPROM.write(1, MAGIC >> 8);
	EEPROM.write(2, _cbor->length() & 0xFF);
	EEPROM.write(3, _cbor->length() >> 8);
	_cbor->offset(0);
	for (int i = 0; i < _cbor->length(); i++) {
		EEPROM.write(i + 4, _cbor->read());
	}
}

void ConfigClass::menu() {
	Serial.begin(115200);

	while (true) {

	}

}

ConfigClass Config;

