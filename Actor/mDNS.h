/*
 * mDNS.h
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#ifndef MDNS_H_
#define MDNS_H_
#include <Actor.h>
#include <Wifi.h>
#include <ESP8266mDNS.h>
class mDNS :public Actor {
	Wifi* _wifi;
public:
	mDNS();
	virtual ~mDNS();
	void on(Header);
	void loop();
	void setup(Wifi* src);
};

#endif /* MDNS_H_ */
