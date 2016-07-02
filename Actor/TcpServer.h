/*
 * TcpServer.h
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include <Actor.h>
#include <ESP8266WiFi.h>
#include <TcpClient.h>

class TcpClient;
class TcpServer :public Actor,public WiFiServer {
	TcpClient* _tcpClient;
public:
	TcpServer(uint16_t port);
	virtual ~TcpServer();
	void on(Header);
	void loop();
	void setup(TcpClient* client);
};

#endif /* TCPSERVER_H_ */
