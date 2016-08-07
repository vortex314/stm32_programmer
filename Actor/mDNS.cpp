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
	if (!MDNS.begin("esp8266")) {
		Log.printf("Error setting up MDNS responder!");
	}
}

void mDNS::loop() {
	MDNS.update();
}

