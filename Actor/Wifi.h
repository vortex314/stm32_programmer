/*
 * Wifi.h
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <Actor.h>
#include <ESP8266WiFi.h>

class Wifi: public Actor {
	bool _connected;
	String _ssid;
	String _password;
	enum {
		DISCONNECTED, CONNECTED
	} _state;
public:
	Wifi();

	virtual ~Wifi();
	void init();
	void loop();
	inline bool connected() {
		return _connected;
	}
	void state(uint32_t st);
	void setConfig(String& ssid,String& password);

};

#endif /* WIFI_H_ */
