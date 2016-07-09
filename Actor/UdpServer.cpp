/*
 * UdpServer.cpp
 *
 *  Created on: Jul 8, 2016
 *      Author: lieven
 */

#include "UdpServer.h"

#define BUFFER_SIZE 300

UdpServer::UdpServer(uint16_t port) :
		Actor("UdpServer"), WiFiUDP(), _cbor_request(BUFFER_SIZE), _cbor_response(
		BUFFER_SIZE) {
	_local_port = port;
	_remote_port = 0;
}

UdpServer::~UdpServer() {

}

extern "C" {  //required for read Vdd Voltage
#include "user_interface.h"
// uint16 readvdd33(void);
}

void serialSwap() {
	static bool swapped = false;
	if (!swapped) {
		Serial.swap();
		swapped = true;
	}
}

void logToSyslog(char* data, uint32_t length) {
	WiFiUDP udp;
	IPAddress ip = IPAddress(192, 168, 0, 223);
	udp.beginPacket(ip, 514);
	// <14> : user level = 1 *8 + informational = 6
	uint32_t millisec = millis();
	uint32_t sec = millisec / 1000;
	uint32_t min = sec / 60;
	uint32_t hour = min / 60;
	millisec -= sec * 1000;
	sec -= min * 60;
	min -= hour * 60;
	udp.printf(
			"<14>1 2016-07-09T%02d:%02d:%02d.%03dZ esp8266.local stm32_programmer - - - ",
			hour, min, sec, millisec, system_get_chip_id());
	udp.write(data, length);
	udp.write('\n');
	udp.endPacket();
	yield();
}

void UdpServer::setup() {
	begin(_local_port);	// listen on port 3881

}

void UdpServer::on(Header hdr) {
	if (hdr.is(REPLY(CONNECT))) {
		LOGF(" switching to syslog");
		delay(10);
		Log.setOutput(logToSyslog);
		serialSwap();
	} else if (hdr.is(Event::TIMEOUT)) {
		timeout(100);
	} else if (hdr.is(Event::TXD)) {

	}
}

void UdpServer::send(IPAddress ip, uint16_t port, uint8_t* data,
		uint16_t length) {
	beginPacket(ip, port);
	write(data, length);
	endPacket();
}
#include <Stm32.h>

void UdpServer::loop() {
	if (int noBytes = parsePacket()) {
		LOGF(" udp[%d] received from %s:%d ", noBytes,
				_remote_ip.toString().c_str(), _remote_port);
		_remote_port = remotePort();
		_remote_ip = remoteIP();
		_cbor_request.clear();
		for (int i = 0; i < noBytes; i++) {
			_cbor_request.write(read());
		}
		Stm32::_gStm32->handle(_cbor_request, _cbor_response);
		LOGF(" udp[%d] send to %s:%d ", _cbor_response.length(),
				_remote_ip.toString().c_str(), _remote_port);
		send(_remote_port, _remote_ip, _cbor_response.data(),
				_cbor_response.length());
	}
}

