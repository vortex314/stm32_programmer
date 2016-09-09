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

IPAddress mDNS::query(const char* service) {
	for(int i=0;i< 5;i++) {
		LOGF(" looking for MQTT host ");

		int number = MDNS.queryService(service, "tcp");

		if (number > 0) {

			for (uint8_t result = 0; result < number; result++) {
				int port = MDNS.port(result);
				String host = MDNS.hostname(result);
				IPAddress IP = MDNS.IP(result);
				LOGF("Service Found [%u] %s (%u.%u.%u.%u) port = %u\n", result,
						host.c_str(), IP[0], IP[1], IP[2], IP[3], port);
				return IP;
			}
		}
	}
	return IPAddress(37,187,106,16);	// test.mosquitto.org
}

