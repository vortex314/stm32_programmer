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


class Stm32 : public Actor ,public TcpClient {
	WiFiClient _tcp;
	Slip _slip;
	Cbor _cbor;
	Bytes _scenario;
	Bytes _rxd;
	int _id;
	enum { READY,EXECUTING } _state;
	enum Boot0Mode { USER,BOOTLOADER} ;
	bool receive();
	void reset();
	void boot0(Boot0Mode );
	void reply(int cmd,int id,int error,Bytes& data);
public:
	Stm32();
	virtual ~Stm32();
	void on(Header);
	void loop();
	void setup(WiFiServer* srv);
};

#endif /* STM32_H_ */
