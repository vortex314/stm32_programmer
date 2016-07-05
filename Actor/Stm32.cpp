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

Str line(100);

Stm32* _gStm32;

Stm32::Stm32() :
		Actor("Stm32"), _slip(300), _cbor(300), _scenario(300), _rxd(300) {
	Actor * actor = new SerialPort();
	_port = actor->ref();
	state(S_INIT);
	_gStm32 = this;
	_connected = false;
}

Stm32::~Stm32() {

}

bool Stm32::connected() {
	return _tcp.connected();
}
Cbor cborLog(400);

void sendLogToTcp(char* data, uint32_t length) {

	if (_gStm32 && _gStm32->connected()) {
		cborLog.clear();
		cborLog.add(Stm32::Cmd::LOG_OUTPUT);
		cborLog.add(0);
		cborLog.add(0);
		cborLog.add((uint8_t*) data, length < 256 ? length : 256);
		Slip::AddCrc(cborLog);
		Slip::Encode(cborLog);
		Slip::Frame(cborLog);
		_gStm32->_tcp.write(cborLog.data(), cborLog.length());
		_gStm32->_tcp.flush();
		yield();
	}
}

void serialSwap() {
	static bool swapped = false;
	if (!swapped) {
		Serial.swap();
		swapped = true;
	}
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
	Log.setOutput(sendLogToTcp);
	serialSwap();
}

void RXD2() {
	Serial.swap();
	return;
	pinMode(3, INPUT);
	pinMode(13, FUNCTION_4); // user UART0-RXD2-D7-GPIO13 for input from stm32, use UART1-TXD1-D4-GPIO2  for output to stm32
}

void RXD0() {
	Serial.swap();
	return;
	pinMode(3, SPECIAL);
	pinMode(13, INPUT); // user UART0-RXD0-RXD-GPIO3-Serial0 for input from stm32, use UART1-TXD1-D4-GPIO2-Serial1  for output to stm32
}

void Stm32::init() {

	pinMode(PIN_RESET, OUTPUT);
	digitalWrite(PIN_RESET, 1);
	pinMode(PIN_BOOT0, OUTPUT);
	boot0(USER);
	reset();
//	Log.setOutput(sendLogToTcp);
//	Serial.swap();
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

#define DELAY 10

void Stm32::sendTcp(int cmd, int id, int error, Bytes& data) {
	_cbor.clear();
	_cbor.add(cmd);
	_cbor.add(id);
	_cbor.add(error);
	_cbor.add(data);

	LOGF(" reply cbor %s", _cbor.toHex(line.clear()));
	_slip.write(&_cbor);
	_slip.addCrc();
	_slip.encode();
	_slip.frame();
	_tcp.write(_slip.data(), _slip.length());
	_tcp.flush();
	_slip.reset();
	yield();
}

void Stm32::engine() {
	//					Log::disable();
	//					delay(10);
	//					Serial.swap();
	_scenario.offset(0);
	uint8_t instr;
	_rxd.clear();
	while (Serial.available()) { // flush read buffer
		_rxd.write(Serial.read());
	}

	LOGF(" scenario %s", _scenario.toHex(line.clear()));
//	Serial1.write(_scenario.data(),_scenario.length());

	while (_scenario.hasData() && _error == E_OK) {

		instr = _scenario.read();

		switch (instr) {
		case X_WAIT_ACK: {
			LOGF("X_WAIT_ACK");
			timeout(DELAY);
			while (1) {
				if (timeout()) {
					_error = ETIMEDOUT;
					goto END;
				};
				if (Serial.available()) {
					uint8_t b;
					while (Serial.available()) {
						_rxd.write(b = Serial.read());

					}
					if (b == ACK)
						break;
				}
				yield();
			}
			break;
		}
		case X_SEND: {

			LOGF("X_SEND");
			if (!_scenario.hasData()) {
				_error = ENODATA;
				goto END;
			}
			int length = _scenario.read() + 1;
			LOGF("X_SEND %d", length);
			while (length && _scenario.hasData()) {
				Serial.write(_scenario.read());
				length--;
			}
			break;

		}
		case X_RECV: {

			LOGF("X_RECV");
			if (!_scenario.hasData()) {
				_error = ENODATA;
				goto END;
			}
			_lengthToread = _scenario.read();
			timeout(DELAY);
			while (_lengthToread && _error == E_OK) {
				if (timeout()) {
					pinMode(2, INPUT);
					pinMode(13, FUNCTION_4); // user RX(0) for input from stm32, use TX(1) for output to stm32

					_error = ETIMEDOUT;
					goto END;
				}
				while (Serial.available()) {
					_rxd.write(Serial.read());
					_lengthToread--;
				}
				yield();
			}
			break;

		}
		case X_RECV_VAR: {

			LOGF("X_RECV_VAR");
			uint8_t b;
			timeout(DELAY);
			while (1) {
				if (timeout()) {
					_error = ETIMEDOUT;
					goto END;
				}
				if (Serial.available()) {
					_rxd.write(b = Serial.read());
					break;
				}
				yield();

			}
			LOGF("X_RECV_VAR : %d", b);
			_lengthToread = b;
			timeout(DELAY);
			while (_lengthToread && _error == E_OK) {
				if (timeout()) {
					_error = ETIMEDOUT;
					goto END;
				}
				while (Serial.available()) {
					_rxd.write(Serial.read());
					_lengthToread--;
				}
				yield();
			}
			break;
		}
		}
	}
	END: return;
}

void Stm32::on(Header hdr) {
	if (hdr.is(INIT))
		init();
	if (_tcp.available()) {
		if (receive()) {
			int cmd;
			_error=E_OK;
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
					LOGF(" scenario : %d", _scenario.length());
					engine();
				}
			} else {
				LOGF(" no cbor data found ");
			}
			sendTcp(cmd, _id, _error, _rxd);
			_rxd.clear();
		} else {
			LOGF(" incomplete data ");
		}
	}
	if (Serial.available()) {
		while (Serial.available())
			_rxd.write(Serial.read());
		LOGF(" received : %s", _rxd.toHex(line.clear()));
		_rxd.clear();
	}
}

