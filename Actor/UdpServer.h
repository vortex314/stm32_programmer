/*
 * UdpServer.h
 *
 *  Created on: Jul 8, 2016
 *      Author: lieven
 */

#ifndef ACTOR_UDPSERVER_H_
#define ACTOR_UDPSERVER_H_

#include <Actor.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Cbor.h>

class UdpServer : public Actor,public WiFiUDP {
	uint16_t _local_port;
	IPAddress _remote_ip;
	uint16_t _remote_port;
	Cbor _cbor_request;
	Cbor _cbor_response;
public:
	UdpServer(uint16_t);
	virtual ~UdpServer();
	void on(Header);
	void loop();
	void setup();
	void send(IPAddress ,uint16_t,uint8_t*,uint16_t );
};

#endif /* ACTOR_UDPSERVER_H_ */
