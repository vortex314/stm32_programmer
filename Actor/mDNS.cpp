/*
 * mDNS.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "mDNS.h"

mDNS::mDNS() :
		Actor("MDNS") {
}

mDNS::~mDNS() {
}

void mDNS::onWifiConnected(Header hdr) {
	if (!MDNS.begin(WiFi.hostname().c_str())) {
		LOGF("Error setting up MDNS responder!");
	}
	MDNS.addService("wibo", "udp", 1883);
}

void mDNS::loop() {
	MDNS.update();
}

