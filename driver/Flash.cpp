/*
 * Flash.cpp
 *
 *  Created on: Oct 1, 2015
 *      Author: lieven
 *      xtensa-lx106-elf-g++ -nostartfiles -nodefaultlibs -nostdlib -L"/home/lieven/workspace/Common/Debug" -L/home/lieven/esp_iot_sdk_v1.4.0/lib -Lgcc -u call_user_start -Wl,-static -T../ld/link.ld  -Wl,--gc-sections -mlongcalls -Xlinker -L/home/lieven/esp-open-sdk/sdk/lib -Xlinker -lssl -lmain -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lssl -lmain -lc -Xlinker --gc-sections -Xlinker --no-undefined -o "esp_cbor"  ./wifi/Tcp.o ./wifi/Wifi.o  ./mqtt/Mqtt.o ./mqtt/MqttFramer.o ./mqtt/MqttMsg.o ./mqtt/Topic.o  ./cpp/Flash.o ./cpp/Gpio.o ./cpp/LedBlink.o ./cpp/Pump.o ./cpp/Stm32.o ./cpp/Sys.o ./cpp/Topics.o ./cpp/UartEsp8266.o ./cpp/stubs.o  ./config.o ./gpio.o ./gpio16.o ./mutex.o ./uart.o ./user_main.o ./util.o ./utils.o ./watchdog.o  -lCommon -lm -lssl -llwip -lwpa -lnet80211 -lphy -lpp -lmain -lc -lhal -lgcc
 *
 *-nostartfiles -nodefaultlibs -nostdlib -L"/home/lieven/workspace/Common/Debug"
 *-nostartfiles -L/home/lieven/esp_iot_sdk_v1.4.0/lib -Lgcc -u call_user_start
 *-nostartfiles -Wl,-static -T../ld/link.ld  -Wl,--gc-sections -mlongcalls -Xlinker
 *-nostartfiles -L/home/lieven/esp-open-sdk/sdk/lib -Xlinker -lssl -lmain -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lssl -lmain
 *-nostartfiles -lc -Xlinker --gc-sections -Xlinker --no-undefined
 *
 PAGE LAYOUT :
 <MAGIC:32><SEQUENCE;32><itemIndex:16+length:16><data..length rounded 4 ><itemIndex+length><data:n><0xFFFFFFFF>
 item idx==even : key
 item idx+1==uneven : value of idx+1
 Bin
 DownloadHeader() address
 mem size         512KB 1024KB 2048KB 4096KB
 esp_init_data_default.bin 0x7C000 0xFC000 0x1FC000 0x3FC000
 blank.bin 0x7E000 0xFE000 0x1FE000 0x3FE000
 boot.bin 0x00000 0x00000 0x00000 0x00000

 */

extern "C" {
#include "espmissingincludes.h"
#include "esp8266.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "debug.h"
#include "user_config.h"
#include "queue.h"
#include "ets_sys.h"
}
#include "stdint.h"
#include "Flash.h"
#include "Sys.h"


uint32_t roundQuad(uint32_t value) {
	if (value & 0x3)
		return (value & 0xFFFFFFFC) + 0x4;
	else
		return value;
}

Flash::Flash() {
	_keyMax = 0;
	_freePos = 0;
	_page = 0;
	_sequence = 0;
	_highestIndex = 0;
}

Flash::~Flash() {
}

void Flash::init() {
	findOrCreateActivePage();
	_page = PAGE_START + PAGE_SIZE * (_sequence % PAGE_COUNT);
	LOGF("page address  : 0x%X ", _page);
	LOGF("free start  : 0x%X", _freePos);
	LOGF("sequence  : %d", _sequence);
}

//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

bool Flash::isValidPage(uint32_t page, uint32_t& sequence) {
	uint32_t magic;
	uint32_t index = 0;
	Erc erc = read(page, &magic);
	if (erc)
		return false;
	if (magic != PAGE_SIGNATURE) {
		return false;
	}
	erc = read(page + 4, &index);
	if (erc)
		return false;
	sequence = index;
	return true;
}

//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

Erc Flash::findOrCreateActivePage() {
	_sequence = 0;
	uint32_t sequence = 0;

	for (uint32_t i = 0; i < PAGE_COUNT; i++) {
		if (isValidPage(PAGE_START + i * PAGE_SIZE, sequence)
				&& sequence > _sequence) {
			_page = PAGE_START + i * PAGE_SIZE;
			_sequence = sequence;
		}
	}
	if (_sequence == 0) {
		initPage(0);
		_page = PAGE_START;
		_sequence = 0;
	}
	uint32_t addr = _page + 8;
	Quad q;
	Erc erc;
	while (true) {
		erc = read(addr, &q.w);
		if (erc)
			return erc;
		if (q.index == 0xFFFF)
			break;
		addr += 4 + roundQuad(q.length);
		if (addr > (_page + PAGE_SIZE))
			break;
	}
	_freePos = addr;
	return E_OK;
}
//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

Erc Flash::read(uint32_t address, uint32_t* data) { // read QUad at quad boundary and cache
	static uint32_t w;       // last word read
	static uint32_t lastAddr = 0;   // last address where word was read
	if (lastAddr != address) {
		if (spi_flash_read(address, &w, 4) != SPI_FLASH_RESULT_OK)
			return EINVAL;
		lastAddr = address;
	}
	*data = w;
//      INFO("@ 0x%X : 0x%X", addr, w);
	return E_OK;
}

//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

Erc Flash::read(uint32_t address, uint8_t* pb) {
	Quad W;
	Erc erc;
	if ((erc = read(address & 0xFFFFFFFC, &W.w)) != E_OK)
		return erc; // take start int32
	*pb = W.b[address & 0x03];
	return E_OK;
}

//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

Erc Flash::write(uint32_t address, uint32_t w) {
//      INFO("@ 0x%X : 0x%X", address, w);
	if (spi_flash_write(address, &w, 4) != SPI_FLASH_RESULT_OK)
		return EINVAL;
	return E_OK;
}

//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

Erc Flash::initPage(uint32_t sequence) {
	uint32_t page = PAGE_START + PAGE_SIZE * (sequence % PAGE_COUNT);
	Erc erc = write(page, PAGE_SIGNATURE);
	if (erc)
		return erc;
	return write(page + 4, sequence);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

Erc Flash::put(uint16_t index, uint8_t* data, uint16_t length) {
	Quad q;
	q.index = index;
	q.length = length;
	Erc erc = write(_freePos, q.w);
	if (erc)
		return erc;
	for (uint16_t i = 0; i < length + 3; i += 4) {
		q.w = 0xFFFFFFFF;
		for (uint32_t j = 0; i + j < length && j < 4; j++)
			q.b[j] = data[i + j];
		erc = write(_freePos + 4 + i, q.w);
		if (erc)
			return erc;
	}
	_freePos += 4 + roundQuad(length);
	return E_OK;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

Erc Flash::findIndex(uint16_t index, uint32_t* address) {
	Quad q;
	Erc erc;
	uint32_t addr = _page + 8;
	uint32_t addrIndex = 0;
	while (true) {
		erc = read(addr, &q.w);
		if (erc)
			return erc;
		if (q.index == 0xFFFF)
			break;
		if (_highestIndex < q.index) {
			_highestIndex = q.index;
		}
		if (q.index == index) {
			addrIndex = addr;
		}
		addr += 4 + roundQuad(q.length);
		if (addr > (_page + PAGE_SIZE))
			break;
	}
	if (addrIndex != 0) {
		*address = addrIndex;
		return E_OK;
	}
	*address = 0;
	return ENOENT;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

Erc Flash::findValue(const char* key, uint16_t* index) {
	Quad q;
	Erc erc;
	uint32_t addr = _page + 8;
	uint32_t strLen = strlen(key);
	while (true) {
		erc = read(addr, &q.w);
		if (q.index == 0xFFFF)
			break;
		if (q.length == strLen) {
			char szKey[40];
			uint32_t i;
			uint32_t length = sizeof(szKey);
			for (i = 0; i < q.length && i < length; i++) {
				erc = read(addr + 4 + i, (uint8_t*) (szKey + i));
				if (erc)
					return erc;
			}
			szKey[i] = '\0';
			if ( os_strcmp((const char*) szKey, key) == 0) {
				*index = q.index;
				return E_OK;
			}
		}
		addr += 4 + roundQuad(q.length);
		if (addr > (_page + PAGE_SIZE))
			break;

	}
	return ENOENT;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

Erc Flash::get(uint16_t index, uint8_t* data, uint16_t* maxLength) {
	uint32_t address;
	findIndex(index, &address);
	if (address == 0)
		return ENOENT;
	Quad q;
	Erc erc = read(address, &q.w);
	uint32_t i, j;
	Quad qd;
	for (i = 0; i < q.length; i += 4) {
		erc = read(address + 4 + i, &qd.w);
		if (erc)
			return ENOENT;
		for (j = 0; i + j < q.length; j++) {
			data[i + j] = qd.b[j];
		}
	}
	*maxLength = q.length;
	return E_OK;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
/*


 Erc get(const char* key,char* value,uint16_t* maxLength);*/

Erc Flash::put(const char* key, const char* value) {
	return put(key, (uint8_t*) value, strlen(value));
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

Erc Flash::put(const char* key, uint8_t* value, uint16_t length) {
	uint16_t index;
	Erc erc;
	if (findValue(key, &index) == ENOENT) {
		index = 0xFFFC & (_highestIndex + 2);
		erc = put(index, (uint8_t*) key, strlen(key));
		if (erc)
			return erc;
	}
	erc = put(index + 1, value, length);
	return erc;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

Erc Flash::get(const char* key, uint8_t* value, uint16_t* maxLength) {
	uint16_t len = *maxLength;
	uint16_t idx;
	Erc erc = findValue(key, &idx);
	if (erc)
		return erc;
	erc = get(idx+1, (uint8_t*) value, &len);
	if (erc)
		return erc;
	*maxLength = len;
	return E_OK;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

Erc Flash::get(const char* key, char* value, uint16_t* maxLength) {
	uint16_t len = *maxLength - 1;
	uint16_t idx;
	Erc erc = findValue(key, &idx);
	if (erc)
		return erc;
	erc = get(idx+1, (uint8_t*) value, &len);
	if (erc)
		return erc;
	value[len] = '\0';
	*maxLength = len;
	return E_OK;
}
