/*
 * mDNS.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "mDNS.h"

mDNS::mDNS() : Actor("MDNS"){
	// TODO Auto-generated constructor stub

}

void mDNS::setup(Wifi* wifi){
	_wifi=wifi;
}

mDNS::~mDNS() {
	// TODO Auto-generated destructor stub
}

void mDNS::on(Header hdr) {
	if (hdr.is(_wifi->ref(), REPLY(CONNECT))) {
		if (!MDNS.begin("esp8266")) {
			Log.printf("Error setting up MDNS responder!");
		}
	}
}

void mDNS::loop(){

}

