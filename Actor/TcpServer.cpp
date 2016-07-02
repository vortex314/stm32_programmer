/*
 * TcpServer.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "TcpServer.h"
#define MAX_SRV_CLIENTS 1

WiFiClient wifiClients[MAX_SRV_CLIENTS];

TcpServer::TcpServer(uint16_t port) :
		Actor("TcpServer"), WiFiServer(port) {

}

TcpServer::~TcpServer() {

}

void TcpServer::setup(TcpClient* client) {
	_tcpClient = client;
	begin();
	setNoDelay(true);
}

void TcpServer::on(Header hdr) {
	if (hdr.is(Event::TIMEOUT)) {
		timeout(100);
	}
}

void TcpServer::loop() {
	if (hasClient()) {
		_tcpClient->setup(this);
	}
}
