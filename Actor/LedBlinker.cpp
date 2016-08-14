/*
 * LedBlinker.cpp
 *
 *  Created on: Jul 1, 2016
 *      Author: lieven
 */

#include "LedBlinker.h"

#define PIN 16

LedBlinker::LedBlinker() :
		Actor("LedBlinker") {
	_interval = 1000;
	_isOn = true;
}

LedBlinker::~LedBlinker() {
}

void LedBlinker::init() {
	pinMode(PIN, OUTPUT);
	digitalWrite(PIN, 1);
	timeout(100);
}

void LedBlinker::loop() {
	if (timeout()) {
		if (_isOn) {
			_isOn = false;
			digitalWrite(PIN, 1);
		} else {
			_isOn = true;
			digitalWrite(PIN, 0);
		}
		timeout(_interval);
	}
}

void LedBlinker::blinkFast(Header h) {
	LOGF("");
	_interval = 500;
}

void LedBlinker::blinkSlow(Header h) {
	LOGF("");
	_interval = 2000;
}
