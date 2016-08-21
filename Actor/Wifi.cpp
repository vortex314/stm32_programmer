/*
 * Wifi.cpp
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#include "Wifi.h"
#include <Config.h>

Wifi::Wifi() :
		Actor("Wifi") {
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
			publish(CONNECT);
		} else {
			_connected = false;
			LOGF(" Wifi Disconnected.");
			publish(DISCONNECT);
		}
	}
}


void Wifi::init() {
	char hostString[16] = { 0 };
	sprintf(hostString, "ESP_%06X", ESP.getChipId());
	String hostname;
	Config.get("wifi.hostname", hostname, hostString);
	WiFi.hostname(hostname);
	WiFi.begin(_ssid.c_str(), _password.c_str());
	WiFi.enableSTA(true);
}

void Wifi::loop() {
	state(WiFi.status() == WL_CONNECTED ? CONNECTED : DISCONNECTED);
}

void Wifi::setConfig(String& ssid, String& password) {
	_ssid = ssid;
	_password = password;
}

