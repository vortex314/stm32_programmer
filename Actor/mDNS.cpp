/*
 * mDNS.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "mDNS.h"

mDNS::mDNS() :
		Actor("MDNS") {
	_port = 1883;
	_service = "wibo";
}

mDNS::~mDNS() {
}

void mDNS::setConfig(String& service, uint16_t port) {
	_service = service;
	_port = port;
}

void mDNS::onWifiConnected(Header hdr) {
	if (!MDNS.begin(WiFi.hostname().c_str())) {
		LOGF("Error setting up MDNS responder!");
	}

	MDNS.addService(_service, "udp", _port);
}

void mDNS::loop() {
	MDNS.update();
}


