/*
 * mDNS.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "mDNS.h"
#include <Config.h>

mDNS::mDNS() :
		Actor("MDNS") {
}

mDNS::~mDNS() {
}

void mDNS::onWifiConnected(Header hdr) {
	if (!MDNS.begin(WiFi.hostname().c_str())) {
		LOGF("Error setting up MDNS responder!");
	}
	String service;
	uint32_t port;
	Config.get("udp.port",port,1883);
	Config.get("mdns.service",service,"wibo");
	MDNS.addService(service, "udp", port);
}

void mDNS::loop() {
	MDNS.update();
}

