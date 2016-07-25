/*
 * WifiWebServer.cpp
 *
 *  Created on: Jul 15, 2016
 *      Author: lieven
 */

#include "WifiWebServer.h"

ESP8266WebServer* WifiWebServer::gWifiWebServer = 0;

StaticJsonBuffer<200> buffer;

WifiWebServer::WifiWebServer(uint32_t port) :
		Actor("Webserver"), ESP8266WebServer(port) {
	gWifiWebServer = this;
}

WifiWebServer::~WifiWebServer() {

}

void WifiWebServer::on(Header header) {
	if ( header.is(INIT)) {
		setup();
	}
}

void WifiWebServer::handleRoot() {
	gWifiWebServer->send(200, "text/plain", "hello from esp8266!");
}

void WifiWebServer::handleNotFound() {

	String message = "File Not Found\n\n";
	message += "URI: ";
	message += gWifiWebServer->uri();
	message += "\nMethod: ";
	message += (gWifiWebServer->method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += gWifiWebServer->args();
	message += "\n";
	for (uint8_t i = 0; i < gWifiWebServer->args(); i++) {
		message += " " + gWifiWebServer->argName(i) + ": "
				+ gWifiWebServer->arg(i) + "\n";
	}
	gWifiWebServer->send(404, "text/plain", message);
}

void WifiWebServer::setup(void) {

	ESP8266WebServer::on("/", handleRoot);
	ESP8266WebServer::on("/inline", []() {
		gWifiWebServer->send(200, "text/plain", "this works as well");
	});
	ESP8266WebServer::onNotFound(handleNotFound);
	begin();
}

void WifiWebServer::loop(void) {
	handleClient();
}
