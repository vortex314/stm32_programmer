/*
 * Wifi.cpp
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#include "Wifi.h"

Wifi::Wifi(const char* ssid, const char* password) :
		Actor("Wifi") {
	_ssid = ssid;
	_password = password;
	state(DISCONNECTED);
}

Wifi::~Wifi() {
}

void Wifi::state(uint32_t st) {
	if (st != Actor::state()) {
		Actor::state(st);
		if (st == CONNECTED) {
			_connected = true;
			LOGF(" Wifi Connected.");
			LOGF(" ip : %s ", WiFi.localIP().toString().c_str());
			publish(REPLY(CONNECT));
		} else {
			_connected = false;
			publish(REPLY(DISCONNECT));
			LOGF(" Wifi Disconnected.");
		}
	}
}

void Wifi::setup() {
	WiFi.begin(_ssid, _password);
	LOGF("Connecting to %s ", _ssid);
	WiFi.enableSTA(true);
	state(DISCONNECTED);
}

void Wifi::loop() {
	state(WiFi.status() == WL_CONNECTED ? CONNECTED : DISCONNECTED);
}

