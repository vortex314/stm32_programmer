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
	const char* _ssid;
	const char* _password;
	enum {
		DISCONNECTED, CONNECTED
	} _state;
public:
	Wifi(const char* ssid, const char* password);
	virtual ~Wifi();
	void setup();
	void loop();
	void on(Header);
	inline bool connected() {
		return _connected;
	}
	void state(uint32_t st);

};

#endif /* WIFI_H_ */
