/*
 * Stm32.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "Stm32.h"

Stm32::Stm32() :
		Actor("Stm32"), _slip(300), _cbor(300), _scenario(300), _rxd(300) {
}

Stm32::~Stm32() {

}

void Stm32::setup(WiFiServer* server) {
	WiFiClient serverClient = server->available();
	if (!_tcp.connected()) {
		_tcp.stop();
		_tcp = serverClient;
		LOGF(" serverClient connected : %s",
				_tcp.remoteIP().toString().c_str());
		publish(REPLY(Event::CONNECT));
	} else {
		LOGF(" too many connections ");
		serverClient.stop();
	}

}

bool Stm32::receive() {
	Str line(100);

	while (_tcp.available()) {
		uint8_t b = _tcp.read();
		if (_slip.fill(b)) {
//			_slip.toHex(line.clear());
//			LOGF(" slip : %s ", line.c_str());
//				uint16_t crc = Slip::Fletcher16(_slip.data(), _slip.length()-2);
//				LOGF(" crc calc : %X ",crc);
			if (_slip.isGoodCrc()) {
				_slip.removeCrc();
				_slip.offset(0);
				_cbor.clear();
				_cbor.write(&_slip);
				_cbor.offset(0);
//				_cbor.toHex(line.clear());
//				LOGF(" cbor : %s ", line.c_str());
				int i, id;
				Bytes bytes(0);
				if (_cbor.get(i) && _cbor.get(id) && _cbor.getMapped(bytes)) {
					_cbor.offset(0);
					return true;
//					bytes.toHex(line.clear());
//					LOGF(" cmd=%d , id=%d   ", i, id);
//					LOGF(" data : %s ", line.c_str());
				} else {
					LOGF(" cbor read fails ");
				}
			} else {
				LOGF(" bad CRC ");
			}
			_slip.reset();
		}
	}
	return false;
}

enum Cmd {
	PING, EXEC, RESET, MODE_FLASH, MODE_RUN
};
enum Op {
	X_WAIT_ACK = 0x40,
	X_SEND = 0x41,
	X_RECV = 0x42,
	X_RECV_VAR = 0x43,
	X_RECV_VAR_MIN_1 = 0x44
};
#define DELAY 10

void Stm32::reset() {

}

void Stm32::reply(int cmd, int id, int error, Bytes& data){
_cbor.clear();
_cbor.add(cmd);
_cbor.add(id);
_cbor.add(error);
_cbor.add(data);
_slip.write(&_cbor);
_slip.addCrc();
_slip.encode();
_tcp.write(_slip.data(), _slip.length());
_tcp.flush();
}

void Stm32::on(Header hdr) {
	PT_BEGIN()

	if (hdr.is(me(), Event::RXD)) { // received data
		if (receive()) { // check complete cbor msg
			int cmd;
			_cbor.offset(0);
			if (_cbor.get(cmd) && _cbor.get(_id)) {
				if (cmd == Cmd::PING) {

					Bytes bytes(0);
					reply(Cmd::PING, _id, E_OK, bytes);

				} else if (cmd == Cmd::RESET) {

					reset();
					Bytes bytes(0);
					reply(Cmd::RESET, _id, E_OK, bytes);

				} else if (cmd == Cmd::EXEC && _cbor.get(_scenario)) {

					_scenario.offset(0);
					while (_scenario.hasData()) {
						delay(10);
						Serial.swap(1);
						uint8_t instr = _scenario.read();
						switch (instr) {
						case X_WAIT_ACK: {
							timeout(DELAY);
							PT_WAIT_UNTIL(timeout() || Serial.available());
							if (timeout())
								goto FAILURE;
							break;
						}
						case X_SEND: {
							if (!_scenario.hasData())
								goto FAILURE;
							int length = _scenario.read() + 1;
							while (length-- && _scenario.hasData()) {
								Serial.write(_scenario.read());
							}
							break;
						}
						case X_RECV: {
							if (!_scenario.hasData())
								goto FAILURE;
							static int length = _scenario.read();
							timeout(DELAY);
							while (length-- && !timeout()) {
								PT_WAIT_UNTIL( timeout() || Serial.available());
								if (timeout())
									goto FAILURE;
								_rxd.write(Serial.read());
							}
							break;
						}
						}
					}

				}
			}
		}
		FAILURE: {
			Bytes bytes(0);
			reply(Cmd::RESET, _id, ETIMEDOUT, bytes);
		}
		Serial.swap(0);
	}
PT_END();
}

void Stm32::loop() {
if (_tcp.available()) {
	publish (RXD);
}
}
