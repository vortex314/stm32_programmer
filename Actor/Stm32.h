/*
 * Stm32.h
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#ifndef STM32_H_
#define STM32_H_

#include <Actor.h>
#include <Arduino.h>
#include <pins_arduino.h>
#include <WiFiClient.h>
#include <TcpClient.h>
#include <Cbor.h>
#include <Slip.h>

class SerialPort : public Actor {
public:
	SerialPort() : Actor("SerialPort") {

	}
	void loop() {
		if ( Serial.available())
			publish(RXD);
	}
};


class Stm32 : public Actor ,public TcpClient {
	WiFiClient _tcp;
	ActorRef _port;
	Slip _slip;
	Cbor _cbor;
	Bytes _scenario;
	Bytes _rxd;
	int _id;
	uint32_t _lengthToread;
	uint32_t _error;
	enum State { S_INIT,S_READY,S_EXECUTING } _state;
	enum Boot0Mode { USER,BOOTLOADER} ;
	bool receive();
	void reset();
	void boot0(Boot0Mode );
	void reply(int cmd,int id,int error,Bytes& data);
	void init();
	void engine();
public:
	Stm32();
	virtual ~Stm32();
	void on(Header);
	void loop();
	void setup(WiFiServer* srv);
};

#endif /* STM32_H_ */
