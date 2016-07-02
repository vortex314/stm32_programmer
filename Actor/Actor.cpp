/*
 * Actor.cpp
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#include "Actor.h"

class Dummy: public Actor {
public:
	Dummy() :
			Actor("Dummy") {
	}
	void on(Header h) {
		logHeader(__FUNCTION__, h);
	}

};

Actor* Actor::_actors[MAX_ACTORS];
uint32_t Actor::_count = 0;
Actor* Actor::_gDummy = new Dummy();
QueueTemplate<Header> Actor::_queue(20);

const char*strEvent[] = { "INIT", "TIMEOUT", "STOP", "RESTART", "CONFIG", "TXD",
		"RXD", "CONNECT", "DISCONNECT", "ADD_LISTENER", "PUBLISH", "SUBSCRIBE",
		"NONE" };

char sEvent[30];

char* strcat(char *dst, const char* src) {
	while (*dst)
		dst++;
	while (*src) {
		*dst = *src;
		dst++;
		src++;
	}
	*dst = '\0';
	return dst;
}

const char* Actor::eventToString(uint8_t event) {
	if ((event & 0x7F) > sizeof(strEvent)) {
		return "UNKNOWN";
	}
	sEvent[0] = '\0';
	if (event & 0x80) {
		strcpy(sEvent, "REPLY(");
	};

	strcat(sEvent, strEvent[event & 0x7F]);
	if (event & 0x80) {
		strcat(sEvent, ")");
	};
	return sEvent;
}

Actor::Actor(const char* name) {
	_timeout = UINT_LEAST64_MAX;
	_name = name;
	_me = _count++;
	_actors[_me] = this;
	LOGF(" Actor : %s [%d]", _name, _me);
}

void Actor::logHeader(const char* prefix, Header h) {
	LOGF(" %s EVENT %s => %s => %s ",
			prefix, actor(h._src).name(), eventToString(h._event), actor(h._dst).name());
}

void Actor::on(Header h) {
	LOGF(" me : %s ", _name);
	logHeader(__FUNCTION__, h);
}

void Actor::eventLoop() {
	Header hdr;
	while (_queue.get(hdr) == E_OK) {
		logHeader(__FUNCTION__, hdr);
		for (int i = 1; i < _count; i++) {
			_actors[i]->on(hdr);
		}
	}
	for (int i = 1; i < _count; i++) {
		_actors[i]->loop();
		if (_actors[i]->timeout()) {
			_actors[i]->on(Header(_actors[i]->me(),_actors[i]->me(),TIMEOUT,0));
		}
	}
}
void Actor::publish(Event ev) {
	Header h((ActorRef) 0, me(), ev, 0);
	_queue.put(h);
}

Actor::~Actor() {

}

bool Header::is(int dst, int src, Event event, uint8_t detail) {
	if (dst == ANY || dst == _dst) {
		if (src == ANY || src == _src) {
			if (event == ANY || event == _event) {
				return true;
			}
		}
	}
	return false;
}

bool Header::is(ActorRef ref, Event event) {
	if (ref == _src) {
		if (event == _event) {
			return true;
		}
	}
	return false;
}

bool Header::is(Event event, uint8_t detail) {

	if (event == ANY || event == _event) {
		if (detail == ANY || detail == _detail) {
			return true;
		}
	}
	return false;
}

bool Header::is(ActorRef dst, ActorRef src, Event event, uint8_t detail) {
	if (dst == ANY || dst == _dst) {
		if (src == ANY || src == _src) {
			if (event == ANY || _event == event) {
				return true;
			}
		}
	}
	return false;
}

