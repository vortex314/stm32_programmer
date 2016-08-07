/*
 * Actor.cpp
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#include "Actor.h"

HandlerEntry Actor::_handlers[30];
uint32_t Actor::_handlerCount = 0;
QueueTemplate<Header> Actor::_queue(20);
Actor* Actor::_actors[15];
uint32_t Actor::_actorCount = 0;

SystemClass System;

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
	_id = _actorCount++;
	ASSERT_LOG(_actorCount < (sizeof(_actors) / sizeof(Actor*)));
	_actors[_id] = this;
	_state = 0;
	_ptLine = LineNumberInvalid;
	LOGF(" Actor ctor : %s [%d]", _name, _id);
}

void Actor::logHeader(const char* prefix, Header h) {
	const char* src = h._src < _actorCount ? _actors[h._src]->_name : "ANY";
	const char* dst = h._dst < _actorCount ? _actors[h._dst]->_name : "ANY";
	const char* event =
			h._event < MAX_EVENTS ? eventToString(h._event) : " UNKNOWN EVENT";

	LOGF(" %s - %s:%s => %s ", prefix, src, event, dst);
}

void Actor::on(Header h, EventHandler m) {
	_handlers[_handlerCount]._filter = h;
	_handlers[_handlerCount]._method = m;
	logHeader(__FUNCTION__, h);
}

void Actor::on(Event event, Actor& actor, EventHandler m) {
	_handlers[_handlerCount]._filter = Header(id(), event);
	_handlers[_handlerCount]._method = m;
	_handlers[_handlerCount]._actor = &actor;
	ASSERT_LOG(_handlerCount < (sizeof(_handlers) / sizeof(HandlerEntry)));
	_handlerCount++;
}

void Actor::initAll() {
	for (int i = 0; i < _actorCount; i++)
		_actors[i]->init();
}

bool Actor::match(Header src, Header filter) {
//	LOGF(" comparing headers src : %X filter : %X", src._word, filter._word);
	if (filter._dst == ANY || filter._dst == src._dst)
		if (filter._src == ANY || filter._src == src._src)
			if (filter._event == ANY || filter._event == src._event)
				return true;
	return false;
}

void Actor::eventLoop() {
	Header hdr;
	// Check event in Queue
	while (_queue.get(hdr) == E_OK) {
//		LOGF("hdr:%X", hdr);
//		logHeader(" event on queue :", hdr);
		for (int i = 0; i < _handlerCount; i++) {
//			logHeader(" filter : ", _handlers[i]._filter);
			if (match(hdr, _handlers[i]._filter)) {
				LOGF(" calling handler ! %s ", _handlers[i]._actor->name());
				delay(10);
				CALL_MEMBER_FN(*_handlers[i]._actor,_handlers[i]._method)(hdr);
			}
		}
	}
	for (int i = 0; i < _actorCount; i++) {
		_actors[i]->loop();
		if (_actors[i]->timeout()) {
			for (int j = 0; j < _handlerCount; j++) {
				if (match(Header(_actors[i]->id(), TIMEOUT),
						_handlers[i]._filter)) {
					CALL_MEMBER_FN(*_handlers[i]._actor,_handlers[i]._method)(
							hdr);
				}
			}
		}
	}
}
void Actor::publish(uint8_t ev) {
	Header h((uint8_t) ANY, id(), (Event) ev, 0);
	publish(h);
}

void Actor::publish(Header h) {
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

