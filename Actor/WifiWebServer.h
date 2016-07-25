/*
 * WifiWebServer.h
 *
 *  Created on: Jul 15, 2016
 *      Author: lieven
 */

#ifndef ACTOR_WIFIWEBSERVER_H_
#define ACTOR_WIFIWEBSERVER_H_

#include <Actor.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

class WifiWebServer: public Actor, ESP8266WebServer {
	static ESP8266WebServer* gWifiWebServer;
public:
	WifiWebServer(uint32_t port);
	virtual ~WifiWebServer();
	void on(Header);
	void loop();
	void setup(void);
	void init();
	static void handleRoot();
	static void handleNotFound();
};

#endif /* ACTOR_WIFIWEBSERVER_H_ */
