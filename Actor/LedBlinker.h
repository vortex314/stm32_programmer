/*
 * LedBlinker.h
 *
 *  Created on: Jul 1, 2016
 *      Author: lieven
 */

#ifndef LEDBLINKER_H_
#define LEDBLINKER_H_

#include <Actor.h>
#include <Arduino.h>

class LedBlinker : public Actor {
	uint32_t _interval;
	bool _isOn;
	ActorRef _src;
public:
	LedBlinker();
	virtual ~LedBlinker();
	void on(Header);
	void loop();
	void setup(ActorRef src);
};

#endif /* LEDBLINKER_H_ */
