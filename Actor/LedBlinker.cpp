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

void LedBlinker::setup(ActorRef src) {
	LOGF(" src : %d %s ",src,actor(src).name());
	_src = src;
	pinMode(PIN, OUTPUT);
	digitalWrite(PIN, 1);
	timeout(100);
}

void LedBlinker::loop(){

}

void LedBlinker::on(Header hdr) {
	if (hdr.is(TIMEOUT)) {
		if (_isOn) {
			_isOn = false;
			digitalWrite(PIN, 1);
		} else {
			_isOn = true;
			digitalWrite(PIN, 0);
		}
		timeout(_interval);
	} else if (hdr.is(_src, REPLY(CONNECT))) {
		_interval = 500;
	} else if (hdr.is(_src, REPLY(DISCONNECT))) {
		_interval = 100;
	}
}
