#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <pins_arduino.h>
#include <Sys.h>
#include <Wifi.h>
#include <LedBlinker.h>
#include <TcpServer.h>
#include <TcpClient.h>
#include <ESP8266mDNS.h>
#include <Stm32.h>
#include <mDNS.h>
#include <ctype.h>
#include <uart.h>
int islower(int a) {
	if (a >= 'a' || a <= 'z')
		return true;
	return false;
}
int tolower(int a) {
	if (islower(a))
		return a;
	return a - 'A' + 'a';
}

//how many clients should be able to telnet to this ESP8266

Wifi wifi("Merckx", "LievenMarletteEwoutRonald");
LedBlinker ledBlinker;
TcpServer tcpServer(23);

Stm32 stm32;
mDNS mdns;

extern "C" void setup() {
	Serial.begin(115200, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL);
	Serial1.begin(115200, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_TX_ONLY);
	wifi.setup();
	ledBlinker.setup(wifi.ref());
	tcpServer.setup(&stm32);
	mdns.setup(&wifi);
	Actor::pub(INIT);
	return;
}

extern "C" void loop() {
	Actor::eventLoop();
	return;
	uint8_t i;
	//check if there are any new clients

	//check UART for data
	/*	if (Serial.available()) {
	 size_t len = Serial.available();
	 uint8_t sbuf[len];
	 Serial.readBytes(sbuf, len);
	 //push UART data to all connected telnet clients
	 for (i = 0; i < MAX_SRV_CLIENTS; i++) {
	 if (serverClients[i] && serverClients[i].connected()) {
	 serverClients[i].write(sbuf, len);
	 delay(1);
	 }
	 }
	 }*/
}
