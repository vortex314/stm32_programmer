/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 David Ogilvy (MetalPhreak)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Spi.h>
#include <Sys.h>
#include "espmissingincludes.h"

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_init
//   Description: Wrapper to setup HSPI/SPI GPIO pins and default SPI clock
//    Parameters: spi_no - SPI (0) or HSPI (1)
//
////////////////////////////////////////////////////////////////////////////////

Spi* Spi::gSpi1 = new Spi(HSPI);

Spi::Spi(uint8 spi_no) {
	_spi_no = spi_no;
	ASSERT_LOG(_spi_no <= 1)
	_mode = 0;
	_duplex = true;
	_highestBitFirst = true;
	_highestByteFirst = false;
}

uint32_t add(uint32_t value, uint32_t mask, uint32_t shift) {
	ASSERT_LOG(shift < 32);
	return (value & mask) << shift;
}

#define SET(reg,mask,value) *(volatile uint32_t *)reg =  (*(uint32_t*)(reg) & (~(mask << mask ## _S))) | (( (value) & mask ) << mask ## _S)
#define CLEAR(reg) *(volatile uint32_t *)reg=0
#define SET_BIT(reg , bit) *(volatile uint32_t *)reg = (*(volatile uint32_t *)reg) | (bit)
#define CLR_BIT(reg,bit) *(volatile uint32_t *)reg = (*(volatile uint32_t *)reg) & ~(bit)
#define ADD(mask,value) (( value & mask ) << mask ## _S)

void Spi::dwmReset() {
	int pin = 5; // RESET PIN == D1 == GPIO5
	pinMode(pin, 1); // OUTPUT
	delay(10);
	digitalWrite(pin, 0); // PULL LOW
	delay(10); // 10ms
	digitalWrite(pin, 1); // PUT HIGH
}

void Spi::dwmFullSpeed() { // 20 MHz max, 10 Mhz
	SET(SPI_CLOCK(HSPI), SPI_CLKDIV_PRE, 0);
	//
	// divide by 1 ( N+1 )
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_N, 7);
	// 3F max ==
	// count to 16
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_H, 4);
	// 14-7 is high ticks
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_L, 0);

}

void Spi::dwmInit() { // POL/PHA
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
	//Set bit 9 if 80MHz sysclock required
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);
	//GPIO12 is HSPI MISO pin (Master Data In)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);
	//GPIO13 is HSPI MOSI pin (Master Data Out)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);
	//GPIO14 is HSPI CLK pin (Clock)
//    pinMode(SS, OUTPUT); ///< GPIO15
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);
	//GPIO15 is HSPI CS pin (Chip Select / Slave Select)
//	uint32_t reg = 0;

	//------------------------------------------------------ SPI_CLOCK

	CLEAR(SPI_CLOCK(HSPI));
	SET(SPI_CLOCK(HSPI), SPI_CLKDIV_PRE, 8);
	// was 4
	// divide by 5 ( N+1 )
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_N, 15);
	// 3F max ==
	// count to 16
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_H, 8);
	// 14-7 is high ticks
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_L, 0);
	// http://d.av.id.au/blog/hardware-spi-clock-registers should give 1MHz

	//------------------------------------------------------ SPI_USER

	CLEAR(SPI_USER(HSPI));
	SET_BIT(SPI_USER(HSPI), SPI_CS_SETUP);
	// use hardware CS
	SET_BIT(SPI_USER(HSPI), SPI_CS_HOLD);
	if (_highestByteFirst) {
		SET_BIT(SPI_USER(HSPI), SPI_WR_BYTE_ORDER);
		// send from high byte to low
		SET_BIT(SPI_USER(HSPI), SPI_RD_BYTE_ORDER);
		// recv from high byte to low in 32 bit

	}
	if (_mode == 1 || _mode == 2) { // falling edge
		CLR_BIT(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
		// SPI Phase 1 OUT for pol=1 ( falling edge )
		CLR_BIT(SPI_USER(HSPI), SPI_CK_I_EDGE);
		// SPI Phase 1 in for pol==1 , mode 1 or 3
	} else { // mode 0 or 3  rising edge
		SET_BIT(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
		// SPI Phase 0 OUT for pol=0 ( rising edge )
		SET_BIT(SPI_USER(HSPI), SPI_CK_I_EDGE);
		// SPI Phase 0 in for pol==0 , mode 0 or 2
	}

	if (_duplex)
		SET_BIT(SPI_USER(HSPI), SPI_DOUTDIN);
	// set full duplex

	//------------------------------------------------------ SPI_USER1

	CLEAR(SPI_USER1(HSPI));
	SET(SPI_USER1(HSPI), SPI_USR_ADDR_BITLEN, 0);
	SET(SPI_USER1(HSPI), SPI_USR_DUMMY_CYCLELEN, 0);

	//------------------------------------------------------ SPI_USER2

	CLEAR(SPI_USER2(HSPI));
	SET(SPI_USER2(HSPI), SPI_USR_COMMAND_BITLEN, 0);
	SET(SPI_USER2(HSPI), SPI_USR_COMMAND_VALUE, 0);
	//------------------------------------------------------ SPI_CTRL

	CLEAR(SPI_CTRL(HSPI));
	if (_highestBitFirst) {
		CLR_BIT(SPI_CTRL(HSPI), SPI_RD_BIT_ORDER);
		CLR_BIT(SPI_CTRL(HSPI), SPI_WR_BIT_ORDER);
	} else {
		SET_BIT(SPI_CTRL(HSPI), SPI_WR_BIT_ORDER);
		// MSB first in write
		SET_BIT(SPI_CTRL(HSPI), SPI_RD_BIT_ORDER);
		// MSB first in read
	}

	//------------------------------------------------------ SPI_CTRL1
	CLEAR(SPI_CTRL1(HSPI));

	//----------------------------------------------------- SPI_CTRL2
	CLEAR(SPI_CTRL2(HSPI));
	SET(SPI_CTRL2(HSPI), SPI_CS_DELAY_NUM, 0);
	SET(SPI_CTRL2(HSPI), SPI_CS_DELAY_MODE, 1);
	SET(SPI_CTRL2(HSPI), SPI_MOSI_DELAY_NUM, 0); // was 0
	// delay before MISO
	SET(SPI_CTRL2(HSPI), SPI_MOSI_DELAY_MODE, 1); // was 1
	//
	SET(SPI_CTRL2(HSPI), SPI_MISO_DELAY_NUM, 0); // was 0
	// delay before MISO
	SET(SPI_CTRL2(HSPI), SPI_MISO_DELAY_MODE, 1); // was 1
	// sys or spi ticks

	SET(SPI_CTRL2(HSPI), SPI_CK_OUT_HIGH_MODE, SPI_CK_OUT_HIGH_MODE);
	SET(SPI_CTRL2(HSPI), SPI_CK_OUT_LOW_MODE, SPI_CK_OUT_LOW_MODE);

//	LOGF(" SPI_CTRL2 : %X ", *(volatile uint32_t *)(SPI_CTRL2(HSPI)));
	SET(SPI_CTRL2(HSPI), SPI_HOLD_TIME, SPI_HOLD_TIME);
	// set max hold time
	SET(SPI_CTRL2(HSPI), SPI_SETUP_TIME, SPI_SETUP_TIME);
	// set max setup time
//	LOGF(" SPI_CTRL2 : %X ",*(volatile uint32_t*)SPI_CTRL2(HSPI));

/*		WRITE_PERI_REG(SPI_CTRL2(HSPI),
	 (( 0xF & SPI_CS_DELAY_NUM ) << SPI_CS_DELAY_NUM_S) | //
	 (( 0x1 & SPI_CS_DELAY_MODE) << SPI_CS_DELAY_MODE_S) |//
	 (( 0xF & SPI_SETUP_TIME )<< SPI_SETUP_TIME_S ) |//
	 (( 0xF & SPI_HOLD_TIME )<< SPI_HOLD_TIME_S ) |//
	 (( 0xF & SPI_CK_OUT_LOW_MODE )<< SPI_CK_OUT_LOW_MODE_S ) |//
	 (( 0xF & SPI_CK_OUT_HIGH_MODE )<< SPI_CK_OUT_HIGH_MODE_S ) |//
	 (( 0x7 & SPI_MOSI_DELAY_NUM ) << SPI_MOSI_DELAY_NUM_S) |//
	 (( 0x7 & SPI_MISO_DELAY_NUM ) << SPI_MISO_DELAY_NUM_S) |//
	 (( 0x1 & SPI_MOSI_DELAY_MODE )<< SPI_MOSI_DELAY_MODE_S ) |//
	 (( 0x1 & SPI_MISO_DELAY_MODE )<< SPI_MISO_DELAY_MODE_S ) |//
	 0);*/
//		LOGF(" SPI_CTRL2 : %X ",*(volatile uint32_t*)SPI_CTRL2(HSPI));
}

#define MAGIC 0x11111111

void Spi::txf(uint8_t* output, uint8_t outputLength, uint8_t* input,
		uint8_t inputLength) {
	union {
		uint32_t words[16];
		uint8_t bytes[64];
	} out; // assure the uint32_t alignment
	if (outputLength) {
		for (int i = 0; i < 16; i++) {
			out.words[i] = MAGIC;
		}
		memcpy(out.bytes, output, outputLength);
		SET_BIT(SPI_USER(HSPI), SPI_USR_MOSI);
		SET(SPI_USER1(HSPI), SPI_USR_MOSI_BITLEN, (outputLength*8)-1);
	} else {
		CLR_BIT(SPI_USER(HSPI), SPI_USR_MOSI);
		SET(SPI_USER1(HSPI), SPI_USR_MOSI_BITLEN, 0);
	}
	if (inputLength) {
		SET_BIT(SPI_USER(HSPI), SPI_USR_MISO);
		SET(SPI_USER1(HSPI), SPI_USR_MISO_BITLEN, (inputLength*8)-1);
	} else {
		CLR_BIT(SPI_USER(HSPI), SPI_USR_MISO);
		SET(SPI_USER1(HSPI), SPI_USR_MISO_BITLEN, 0);
	}
	ASSERT_LOG( outputLength < 64);
	ASSERT_LOG( inputLength < 64);

//	LOGF(" SPI_USER : %X ", *(volatile uint32_t *)(SPI_USER(HSPI)));
//	LOGF(" SPI_USER1 : %X ", *(volatile uint32_t *)(SPI_USER1(HSPI)));
	for (int i = 0; i < 16; i++) {
		volatile uint32_t* ptr = &SPI1W0;
		*(ptr + i) = MAGIC;
	}

	// txf data to buffers
	volatile uint32_t * fifoPtr = &SPI1W0;
	uint8_t dataSize = ((outputLength + 3) / 4);

	if (outputLength) {
		uint32_t * dataPtr = out.words;
		while (dataSize--) {
			*fifoPtr = *dataPtr;
			dataPtr++;
			fifoPtr++;
		}
	} else {
		// no out data only read fill with dummy data!
		while (dataSize--) {
			*fifoPtr = MAGIC;
			fifoPtr++;
		}
	}
	uint32_t* reg = (uint32_t*) SPI_W0(HSPI);
//	LOGF(" SPI_W0 %d out : %08X %08X %08X %08X %08X",
	//		outputLength, *reg, *(reg+1), *(reg+2), *(reg+3), *(reg+4));


//	cs_select();
	SET_BIT(SPI_CMD(HSPI), SPI_USR);
	// txf data from buffers
	while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR)
		// wait ready
		;
//	cs_deselect();
	if (inputLength) {
		volatile uint8_t * fifoPtr8 = (volatile uint8_t *) &SPI1W0;
		dataSize = inputLength;
		while (dataSize--) {
			*input = *fifoPtr8;
			input++;
			fifoPtr8++;
		}
	}
//	LOGF(" SPI_W0 %d in  : %08X %08X %08X %08X %08X",
	//		inputLength, *reg, *(reg+1), *(reg+2), *(reg+3), *(reg+4));
//	LOGF("------------------------------------------------------------------");

}
/*
void Spi::init() {
	init_gpio(SPI_CLK_USE_DIV);
	clock(SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
	tx_byte_order(SPI_BYTE_ORDER_HIGH_TO_LOW);
	rx_byte_order(SPI_BYTE_ORDER_HIGH_TO_LOW);

	SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_CS_SETUP|SPI_CS_HOLD);
	CLEAR_PERI_REG_MASK(SPI_USER(_spi_no), SPI_FLASH_MODE);

}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_mode
//   Description: Configures SPI mode parameters for clock edge and clock polarity.
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  spi_cpha - (0) Data is valid on clock leading edge
//				             (1) Data is valid on clock trailing edge
//				  spi_cpol - (0) Clock is low when inactive
//				             (1) Clock is high when inactive
//
////////////////////////////////////////////////////////////////////////////////

void Spi::mode(uint8 spi_cpha, uint8 spi_cpol) {
	if (spi_cpha) {
		CLEAR_PERI_REG_MASK(SPI_USER(_spi_no), SPI_CK_OUT_EDGE);
		CLEAR_PERI_REG_MASK(SPI_USER(_spi_no), SPI_CK_I_EDGE);
	} else {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_CK_OUT_EDGE);
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_CK_I_EDGE);
	}



	if (spi_cpol) { // CPOL
		SET_PERI_REG_MASK(SPI_CTRL2(_spi_no),
				SPI_CK_OUT_HIGH_MODE << SPI_CK_OUT_HIGH_MODE_S);
		CLEAR_PERI_REG_MASK(SPI_CTRL2(_spi_no),
				SPI_CK_OUT_LOW_MODE << SPI_CK_OUT_LOW_MODE_S);
	} else {
		SET_PERI_REG_MASK(SPI_CTRL2(_spi_no),
				SPI_CK_OUT_LOW_MODE << SPI_CK_OUT_LOW_MODE_S);
		CLEAR_PERI_REG_MASK(SPI_CTRL2(_spi_no),
				SPI_CK_OUT_HIGH_MODE << SPI_CK_OUT_LOW_MODE_S);
	}
}

void Spi::clear() {
	WRITE_PERI_REG(SPI_W0(_spi_no), 0);

}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_init_gpio
//   Description: Initialises the GPIO pins for use as SPI pins.
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  sysclk_as_spiclk - SPI_CLK_80MHZ_NODIV (1) if using 80MHz
//									 sysclock for SPI clock.
//									 SPI_CLK_USE_DIV (0) if using divider to
//									 get lower SPI clock speed.
//
////////////////////////////////////////////////////////////////////////////////

void Spi::init_gpio(uint8 sysclk_as_spiclk) {

//	if(spi_no > 1) return; //Not required. Valid spi_no is checked with if/elif below.

	uint32 clock_div_flag = 0;
	if (sysclk_as_spiclk) {
		clock_div_flag = 0x0001;
	}

	if (_spi_no == SPI) {
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005 | (clock_div_flag << 8));
		//Set bit 8 if 80MHz sysclock required
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);
	} else if (_spi_no == HSPI) {
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105 | (clock_div_flag << 9));
		//Set bit 9 if 80MHz sysclock required
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);
		//GPIO12 is HSPI MISO pin (Master Data In)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);
		//GPIO13 is HSPI MOSI pin (Master Data Out)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);
		//GPIO14 is HSPI CLK pin (Clock)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);
		//GPIO15 is HSPI CS pin (Chip Select / Slave Select)

	}

}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_clock
//   Description: sets up the control registers for the SPI clock
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  prediv - predivider value (actual division value)
//				  cntdiv - postdivider value (actual division value)
//				  Set either divider to 0 to disable all division (80MHz sysclock)
//
////////////////////////////////////////////////////////////////////////////////

void Spi::clock(uint16 prediv, uint8 cntdiv) {

	if (_spi_no > 1)
		return;

	if ((prediv == 0) | (cntdiv == 0)) {

		WRITE_PERI_REG(SPI_CLOCK(_spi_no), SPI_CLK_EQU_SYSCLK);

	} else {

		WRITE_PERI_REG(SPI_CLOCK(_spi_no),
				(((prediv-1)&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)| (((cntdiv-1)&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)| (((cntdiv>>1)&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)| ((0&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S));
	}

}

#include <Arduino.h>
#include <gpio_c.h>

void Spi::set_hw_cs(bool use) {
	if (use) {
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);
		//GPIO15 is HSPI CS pin (Chip Select / Slave Select)
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_CS_SETUP|SPI_CS_HOLD);
	} else {
		pinMode(15, OUTPUT);
		CLEAR_PERI_REG_MASK(SPI_USER(_spi_no), SPI_CS_SETUP|SPI_CS_HOLD);
	}
}
// D8 == GPIO PIN 15
void Spi::cs_select() {
	digitalWrite(D8, 0);
}

void Spi::cs_deselect() {
	digitalWrite(D8, 1);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_tx_byte_order
//   Description: Setup the byte order for shifting data out of buffer
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1)
//							   Data is sent out starting with Bit31 and down to Bit0
//
//							   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//							   Data is sent out starting with the lowest BYTE, from
//							   MSB to LSB, followed by the second lowest BYTE, from
//							   MSB to LSB, followed by the second highest BYTE, from
//							   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//							   0xABCDEFGH would be sent as 0xGHEFCDAB
//
//
////////////////////////////////////////////////////////////////////////////////

void Spi::tx_byte_order(uint8 byte_order) {

	ASSERT_LOG(_spi_no <= 1)

	if (byte_order) {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_WR_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(_spi_no), SPI_WR_BYTE_ORDER);
	}
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_rx_byte_order
//   Description: Setup the byte order for shifting data into buffer
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1)
//							   Data is read in starting with Bit31 and down to Bit0
//
//							   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//							   Data is read in starting with the lowest BYTE, from
//							   MSB to LSB, followed by the second lowest BYTE, from
//							   MSB to LSB, followed by the second highest BYTE, from
//							   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//							   0xABCDEFGH would be read as 0xGHEFCDAB
//
//
////////////////////////////////////////////////////////////////////////////////

void Spi::rx_byte_order(uint8 byte_order) {

	ASSERT_LOG(_spi_no <= 1)

	if (byte_order) {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_RD_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(_spi_no), SPI_RD_BYTE_ORDER);
	}
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_transaction
//   Description: SPI transaction function
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				  cmd_bits - actual number of bits to transmit
//				  cmd_data - command data
//				  addr_bits - actual number of bits to transmit
//				  addr_data - address data
//				  dout_bits - actual number of bits to transmit
//				  dout_data - output data
//				  din_bits - actual number of bits to receive
//
//		 Returns: read data - uint32 containing read in data only if RX was set
//				  0 - something went wrong (or actual read data was 0)
//				  1 - data sent ok (or actual read data is 1)
//				  Note: all data is assumed to be stored in the lower bits of
//				  the data variables (for anything <32 bits).
//
////////////////////////////////////////////////////////////////////////////////

uint32 Spi::transaction(uint8 cmd_bits, uint16 cmd_data, uint32 addr_bits,
		uint32 addr_data, uint32 dout_bits, uint32 dout_data, uint32 din_bits,
		uint32 dummy_bits) {

	ASSERT_LOG(_spi_no <= 1)

	//code for custom Chip Select as GPIO PIN here

	while (spi_busy(_spi_no))
		; //wait for SPI to be ready

//########## Enable SPI Functions ##########//
	//disable MOSI, MISO, ADDR, COMMAND, DUMMY in case previously set.
	CLEAR_PERI_REG_MASK(SPI_USER(_spi_no),
			SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY);
//	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_DOUTDIN); // LMR set full duplex

	//enable functions based on number of bits. 0 bits = disabled.
	//This is rather inefficient but allows for a very generic function.
	//CMD ADDR and MOSI are set below to save on an extra if statement.
//	if(cmd_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND);}
//	if(addr_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_ADDR);}
	if (din_bits) {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_USR_MISO);
	}
	if (dummy_bits) {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_USR_DUMMY);
	}
//########## END SECTION ##########//

//########## Setup Bitlengths ##########//
	WRITE_PERI_REG(SPI_USER1(_spi_no),
			((addr_bits-1)&SPI_USR_ADDR_BITLEN)<<SPI_USR_ADDR_BITLEN_S | //Number of bits in Address
			((dout_bits-1)&SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S |//Number of bits to Send
			((din_bits-1)&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S |//Number of bits to receive
			((dummy_bits-1)&SPI_USR_DUMMY_CYCLELEN)<<SPI_USR_DUMMY_CYCLELEN_S);
	//Number of Dummy bits to insert
//########## END SECTION ##########//

//########## Setup Command Data ##########//
	if (cmd_bits) {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_USR_COMMAND);
		//enable COMMAND function in SPI module
		uint16 command = cmd_data << (16 - cmd_bits); //align command data to high bits
		command = ((command >> 8) & 0xff) | ((command << 8) & 0xff00); //swap byte order
		WRITE_PERI_REG(SPI_USER2(_spi_no),
				((((cmd_bits-1)&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S) | command&SPI_USR_COMMAND_VALUE));
	}
//########## END SECTION ##########//

//########## Setup Address Data ##########//
	if (addr_bits) {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_USR_ADDR);
		//enable ADDRess function in SPI module
		WRITE_PERI_REG(SPI_ADDR(_spi_no), addr_data << (32 - addr_bits));
		//align address data to high bits
	}

//########## END SECTION ##########//

//########## Setup DOUT data ##########//
	if (dout_bits) {
		SET_PERI_REG_MASK(SPI_USER(_spi_no), SPI_USR_MOSI);
		//enable MOSI function in SPI module
		//copy data to W0
		if (READ_PERI_REG(SPI_USER(_spi_no)) & SPI_WR_BYTE_ORDER) {
			WRITE_PERI_REG(SPI_W0(_spi_no), dout_data << (32 - dout_bits));
		} else {

			uint8 dout_extra_bits = dout_bits % 8;

			if (dout_extra_bits) {
				//if your data isn't a byte multiple (8/16/24/32 bits)and you don't have SPI_WR_BYTE_ORDER set, you need this to move the non-8bit remainder to the MSBs
				//not sure if there's even a use case for this, but it's here if you need it...
				//for example, 0xDA4 12 bits without SPI_WR_BYTE_ORDER would usually be output as if it were 0x0DA4,
				//of which 0xA4, and then 0x0 would be shifted out (first 8 bits of low byte, then 4 MSB bits of high byte - ie reverse byte order).
				//The code below shifts it out as 0xA4 followed by 0xD as you might require.
				WRITE_PERI_REG(SPI_W0(_spi_no),
						((0xFFFFFFFF << (dout_bits - dout_extra_bits) & dout_data) << (8 - dout_extra_bits) | (0xFFFFFFFF >> (32 - (dout_bits - dout_extra_bits))) & dout_data));
			} else {
				WRITE_PERI_REG(SPI_W0(_spi_no), dout_data);
			}
		}
	}
//########## END SECTION ##########//

//########## Begin SPI Transaction ##########//
	SET_PERI_REG_MASK(SPI_CMD(_spi_no), SPI_USR);
//########## END SECTION ##########//

//########## Return DIN data ##########//
	if (din_bits) {
		while (spi_busy(_spi_no))
			; //wait for SPI transaction to complete

		if (READ_PERI_REG(SPI_USER(_spi_no)) & SPI_RD_BYTE_ORDER) {
			return READ_PERI_REG(SPI_W0(_spi_no)) >> (32 - din_bits); //Assuming data in is written to MSB. TBC
		} else {
			return READ_PERI_REG(SPI_W0(_spi_no)); //Read in the same way as DOUT is sent. Note existing contents of SPI_W0 remain unless overwritten!
		}

		return 0; //something went wrong
	}
//########## END SECTION ##########//

	//Transaction completed
	return 1; //success
}*/

/**
 * Set bit order.
 *
 * @param order MSB (1) first or LSB (0) first
 * @see spiOrder_t
 * @return None
 */
/*void Spi::set_bit_order(int order) {

	if (!order) {
		WRITE_PERI_REG(SPI_CTRL(_spi_no),
				READ_PERI_REG(SPI_CTRL(_spi_no)) & (~(SPI_WR_BIT_ORDER | SPI_RD_BIT_ORDER)));
	} else {
		WRITE_PERI_REG(SPI_CTRL(_spi_no),
				READ_PERI_REG(SPI_CTRL(_spi_no)) | (SPI_WR_BIT_ORDER | SPI_RD_BIT_ORDER));
	}

}*/

////////////////////////////////////////////////////////////////////////////////

/*///////////////////////////////////////////////////////////////////////////////
 //
 // Function Name: func
 //   Description:
 //    Parameters:
 //
 ///////////////////////////////////////////////////////////////////////////////*/
/*
 * big endian esp8266
 * 0x12345678 becomes [0x78,0x56,0x34,0x12]
 * SPI fills in same order bytes[3],[2],[1],[0]
 *
 * bytes read are written to : N+3,2,1,0, N+4+3,N+4+2
 *
 *
 */
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
/*void bytesToWords(uint32_t* pW, uint8_t* pB, uint32_t length) {
	typedef union {
		uint32_t word;
		uint8_t bytes[4];
	} Map;
	uint32_t byteIndex, wordIndex;
	Map* pMap = (Map*) pW;
	for (wordIndex = 0; wordIndex < (length / 4) + 1; wordIndex++) {
		uint32_t fraction = length - wordIndex * 4;
		if (fraction > 4)
			fraction = 4;
		pMap->word = 0;
		for (byteIndex = 0; byteIndex < fraction; byteIndex++) {
			pMap->bytes[4 - byteIndex] = pB[byteIndex];
		}
		pMap++;
	}
}*/
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
/*void wordsToBytes(uint32_t* pW, uint8_t* pB, uint32_t length) {
	typedef union {
		uint32_t word;
		uint8_t bytes[4];
	} Map;
	uint32_t byteIndex, wordIndex;
	Map* pMap = (Map*) pW;
	for (wordIndex = 0; wordIndex < (length / 4) + 1; wordIndex++) {
		uint32_t fraction = length - wordIndex * 4;
		if (fraction > 4)
			fraction = 4;

		for (byteIndex = 0; byteIndex < fraction; byteIndex++) {
			pB[byteIndex] = pMap->bytes[4 - byteIndex];
		}
		pMap++;
	}
}*/
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
/*const char* HEX_CHAR = "0123456789ABCDEF";
char line[128];

char* bytesToHex(const uint8_t* pb, uint32_t len) {

	uint32_t offset = 0;
	while ((len > 0) && (offset < (sizeof(line) - 2))) {
		line[offset++] = HEX_CHAR[((*pb) >> 4) & 0xF];
		line[offset++] = HEX_CHAR[*pb & 0xF];
		line[offset++] = ' ';
		len--;
		pb++;
	}
	line[offset++] = '\0';
	return line;
}*/
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
extern "C" int writetospi(uint16 hLen, const uint8 *hbuff, uint32 bLen,
		const uint8 *buffer) {

//	LOGF(" header(%d) : %X %X %X buffer(%d) : %X %X %X ",hLen,hbuff [0],hbuff [1],hbuff [2],bLen,buffer [0],buffer [1],buffer[2]);
	uint8_t output[64];
	ASSERT_LOG((hLen+bLen)<64);
	memcpy(output, hbuff, hLen);
	memcpy(output + hLen, buffer, bLen);
	Spi::gSpi1->txf(output, hLen + bLen, output, 0);
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////

extern "C" int readfromspi(uint16 hLen, const uint8 *hbuff, uint32 bLen,
		uint8 *buffer) {
//	LOGF(" header(%d) : %X %X %X buffer(%d) : %X %X %X ",hLen,hbuff [0],hbuff [1],hbuff [2],bLen,buffer [0],buffer [1],buffer[2]);
	Spi::gSpi1->txf((uint8_t*) hbuff, hLen, buffer, bLen);
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
void Spi::set_rate_low() {
	//------------------------------------------------------ SPI_CLOCK

	CLEAR(SPI_CLOCK(HSPI));
	SET(SPI_CLOCK(HSPI), SPI_CLKDIV_PRE, 4);
	// was 4
	// divide by 5 ( N+1 )
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_N, 15);
	// 3F max ==
	// count to 16
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_H, 8);
	// 14-7 is high ticks
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_L, 0);
	// http://d.av.id.au/blog/hardware-spi-clock-registers should give 1MHz
}
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
void Spi::set_rate_high() {
	SET(SPI_CLOCK(HSPI), SPI_CLKDIV_PRE, 0);
	//
	// divide by 1 ( N+1 )
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_N, 7);
	// 3F max ==
	// count to 16
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_H, 4);
	// 14-7 is high ticks
	SET(SPI_CLOCK(HSPI), SPI_CLKCNT_L, 0);

}

