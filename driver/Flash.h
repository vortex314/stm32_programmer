/*
 * Flash.h
 *
 *  Created on: Oct 1, 2015
 *      Author: lieven
 */

#ifndef FLASH_H_
#define FLASH_H_
#include <Sys.h>
#include <Erc.h>

#include "Config.h"

#define PAGE_SIGNATURE 0xDEADBEEF

#ifdef K512
#define PAGE_START	0x78000
#define PAGE_COUNT 4		// 78000,79000,7A000,7B000
#endif

#define M4
#ifdef M4
#define PAGE_SIZE 0x1000	// 16 * 256 = 4K
#define PAGE_START	0x3F8000
#define PAGE_COUNT 4		// 3F8000,3F9000,3FA000,3FB000
#endif

typedef struct {
	union {
		uint32_t w;
		struct {
			uint16_t index;
			uint16_t length;
		};
		uint8_t b[4];
	};
} Quad;

class Flash {
private:

	uint32_t _page;
	uint32_t _sequence;
	uint32_t _freePos;
	uint16_t _keyMax;
	uint16_t _highestIndex;

	Erc findOrCreateActivePage();
	bool initializePage(uint32_t pageIdx, uint32_t sequence);
	bool scanPage(uint32_t pageIdx);

	bool isValidPage(uint32_t pageIdx, uint32_t& sequence);

public:
	Flash();
	~Flash();
	void init();

	Erc write(uint32_t address,uint32_t word);
	Erc read(uint32_t address,uint32_t* word);
	Erc read(uint32_t address,uint8_t* byte);
	Erc initPage(uint32_t sequence);

	Erc put(uint16_t index,uint8_t* data,uint16_t length);
	Erc get(uint16_t index,uint8_t* data,uint16_t* maxLength);
	Erc findIndex(uint16_t index,uint32_t* address);
	Erc findValue(const char* value,uint16_t* index);

	Erc put(const char* key,const char* value);
	Erc put(const char* key,uint8_t* value,uint16_t length);
	Erc get(const char* key,uint8_t* value,uint16_t* maxLength);
	Erc get(const char* key,char* value,uint16_t* maxLength);

};

#endif /* FLASH_H_ */
