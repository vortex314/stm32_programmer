/*
 * Stm32.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "Stm32.h"

#define PIN_RESET 4 // GPIO4 D2
#define PIN_BOOT0 5 // GPIO5 D1
#define ACK 0x79
#define NACK 0x1F

Stm32::Stm32() :
		Actor("Stm32"), _slip(300), _cbor(300), _scenario(300), _rxd(300) {
	Actor * actor = new SerialPort();
	_port = actor->ref();
	state(S_INIT);
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

void Stm32::init() {
	pinMode(PIN_RESET, OUTPUT);
	digitalWrite(PIN_RESET, 1);
	pinMode(PIN_BOOT0, OUTPUT);
	boot0(USER);
	reset();
}

void Stm32::reset() {
	LOGF("RESET");
	digitalWrite(PIN_RESET, 0);
	delay(10);
	digitalWrite(PIN_RESET, 1);
}

void Stm32::boot0(Boot0Mode bm) {
	if (bm == BOOTLOADER) {
		LOGF("MODE_BOOTLOADER");
		digitalWrite(PIN_BOOT0, 1);
	} else if (bm == USER) {
		LOGF("MODE_USER");
		digitalWrite(PIN_BOOT0, 0);
	}
}

void Stm32::loop() {
	if (_tcp.available()) {
		publish(RXD);
	}
}

bool Stm32::receive() {
	Str line(100);

	while (_tcp.available()) {
		uint8_t b = _tcp.read();
		if (_slip.fill(b)) {
			_slip.toHex(line.clear());
			LOGF(" slip : %s ", line.c_str());
//				uint16_t crc = Slip::Fletcher16(_slip.data(), _slip.length()-2);
//				LOGF(" crc calc : %X ",crc);
			if (_slip.isGoodCrc()) {
				_slip.removeCrc();
				_slip.offset(0);
				_cbor.clear();
				_cbor.write(&_slip);
				_cbor.offset(0);
				_cbor.toHex(line.clear());
				LOGF(" cbor : %s ", line.c_str());
				int i, id;
				Bytes bytes(0);
				if (_cbor.get(i) && _cbor.get(id)) {
					_cbor.offset(0);
					return true;
//					bytes.toHex(line.clear());
//				LOGF(" cmd=%d , id=%d   ", i, id);
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
	PING, EXEC, RESET, MODE_BOOTLOADER, MODE_USER
};
enum Op {
	X_WAIT_ACK = 0x40,
	X_SEND = 0x41,
	X_RECV = 0x42,
	X_RECV_VAR = 0x43,
	X_RECV_VAR_MIN_1 = 0x44
};
#define DELAY 10

void Stm32::reply(int cmd, int id, int error, Bytes& data) {
	_cbor.clear();
	_cbor.add(cmd);
	_cbor.add(id);
	_cbor.add(error);
	_cbor.add(data);
	Str line(100);
	LOGF(" reply cbor %s", _cbor.toHex(line));
	_slip.write(&_cbor);
	_slip.addCrc();
	_slip.encode();
	_slip.frame();
	_tcp.write(_slip.data(), _slip.length());
	_tcp.flush();
	_slip.reset();
}

void Stm32::on(Header hdr) {

	int cmd;
	logHeader("Stm32::on1", hdr);
	uint32_t error = E_OK;
	uint8_t instr;

	PT_BEGIN()
	PT_WAIT_UNTIL(hdr.is(INIT));
	init();
	READY:
	PT_YIELD();

	if (hdr.is(me(), Event::RXD)) { // received data
		logHeader("Stm32::on2", hdr);
		_rxd.clear();
		while (Serial.available())
			Serial.read();
		error = E_OK;
		if (receive()) { // check complete cbor msg
			_cbor.offset(0);
			if (_cbor.get(cmd) && _cbor.get(_id)) {
				LOGF(" cmd : %d , id : %d ", cmd, _id);
				if (cmd == Cmd::PING) {
					// just echo
				} else if (cmd == Cmd::RESET) {

					reset();

				} else if (cmd == Cmd::MODE_BOOTLOADER) {

					boot0(Boot0Mode::BOOTLOADER);

				} else if (cmd == Cmd::MODE_USER) {

					boot0(Boot0Mode::USER);

				} else if (cmd == Cmd::EXEC && _cbor.get(_scenario)) {
					boot0(Boot0Mode::BOOTLOADER);
					reset();
//					Log::disable();
					delay(10);
//					Serial.swap();
					_scenario.offset(0);

					while (_scenario.hasData() && error == E_OK) {
						delay(10);
						instr = _scenario.read();

						if (instr == X_WAIT_ACK) {

							timeout(DELAY);
							PT_WAIT_UNTIL(hdr.is(TIMEOUT) || hdr.is(_port,RXD));
							if (hdr.is(TIMEOUT)) {
								error = ETIMEDOUT;
								goto END;
							} else {
								uint8_t b;
								while (Serial.available()) {
									_rxd.write(b = Serial.read());
								}
								if (b != ACK) {
									error = EINVAL;
									goto END;
								}
							}

						} else if (instr == X_SEND) {

							if (!_scenario.hasData()) {
								error = ENODATA;
								goto END;
							}
							int length = _scenario.read() + 1;
							while (length && _scenario.hasData()) {
								Serial1.write(_scenario.read());
								length--;
							}

						} else if (instr == X_RECV) {

							if (!_scenario.hasData()) {
								error = ENODATA;
								goto END;
							}
							_lengthToread = _scenario.read();
							timeout(DELAY);
							while (_lengthToread-- && error == E_OK) {
								PT_WAIT_UNTIL(
										hdr.is(TIMEOUT) || hdr.is(_port,RXD));
								if (hdr.is(TIMEOUT)) {
									error = ETIMEDOUT;
									goto END;
								}
								while (Serial.available())
									_rxd.write(Serial.read());
							}

						} else if (instr == X_RECV_VAR) {

							uint8_t b;
							timeout(DELAY);
							PT_WAIT_UNTIL(hdr.is(TIMEOUT) || hdr.is(_port,RXD));
							if (hdr.is(TIMEOUT)) {
								error = ETIMEDOUT;
								goto END;
							} else {
								while (Serial.available()) {
									_rxd.write(b = Serial.read());
								}
							}
							_lengthToread = b;
							timeout(DELAY);
							while (_lengthToread && error == E_OK) {
								PT_WAIT_UNTIL(
										hdr.is(TIMEOUT) || hdr.is(_port,RXD));
								if (hdr.is(TIMEOUT)) {
									error = ETIMEDOUT;
									goto END;
								}
								while (Serial.available()) {
									_rxd.write(Serial.read());
									_lengthToread--;
								}
							}
						}
					}
					END: delay(10);
//					Serial.swap();
//					Log::enable();
					timeout(UINT32_MAX);

				}

			} else {
				LOGF(" didn't find cbor fields ");
			}
		} else {
			LOGF(" not complete ");
		}
		reply(cmd, _id, error, _rxd);

	} else if (hdr.is(me(), Event::TXD)) {
	}
	goto READY;
PT_END();
}

