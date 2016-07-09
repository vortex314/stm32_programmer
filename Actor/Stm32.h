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

class SerialPort: public Actor {
public:
	SerialPort() :
			Actor("SerialPort") {

	}
	void loop() {
		if (Serial.available()) {
			publish(RXD);
		}
	}
};

class Stm32: public Actor, public TcpClient {

	ActorRef _port;
	Slip _slip;
	Cbor _cbor;
	Bytes _scenario;
	Bytes _rxd;
	int _id;
	uint32_t _lengthToread;
	uint32_t _error;bool _connected;
	enum State {
		S_INIT, S_READY, S_EXECUTING
	} _state;
	enum Boot0Mode {
		USER, BOOTLOADER
	};bool receive();
	void reset();
	void boot0(Boot0Mode);
	void init();
	void engine();
	void logToTcp();
	void logToSerial();
public:
	static Stm32* _gStm32;
	WiFiClient _tcp;
	enum Cmd {
		PING, EXEC, RESET, MODE_BOOTLOADER, MODE_USER, STM32_OUTPUT, LOG_OUTPUT
	};
	enum Op {
		X_WAIT_ACK = 0x40,
		X_SEND = 0x41,
		X_RECV = 0x42,
		X_RECV_VAR = 0x43,
		X_RECV_VAR_MIN_1 = 0x44
	};
	void handle(Cbor& req, Cbor& reply);
	void sendTcp(int cmd, int id, int error, Bytes& data);
	Stm32();
	virtual ~Stm32();
	void on(Header);
	void loop();
	void setup(WiFiServer* srv);bool connected();
};

#endif /* STM32_H_ */
