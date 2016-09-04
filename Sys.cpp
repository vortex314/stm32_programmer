/*
 * Sys.cpp
 *
 *  Created on: May 15, 2016
 *      Author: lieven
 */
#include <Arduino.h>
#include <Sys.h>

uint32_t Sys::millis(){
	return ::millis();
}

