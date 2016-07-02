/*
 * TcpClient.h
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_
#include <Actor.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>

class TcpClient {
public:
	virtual void setup(WiFiServer* srv)=0;
};

#endif /* TCPCLIENT_H_ */
