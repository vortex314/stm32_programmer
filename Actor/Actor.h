/*
 * Actor.h
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#ifndef ACTOR_H_
#define ACTOR_H_

#include <stdint.h>
#include <Sys.h>
#include <QueueTemplate.h>

typedef enum {
	INIT = 0, TIMEOUT, STOP, RESTART, CONFIG, // detail =0, initial params in payload, =1 , load config from flash again
	TXD,
	RXD,
	CONNECT,
	DISCONNECT,
	ADD_LISTENER,
	PUBLISH,
	SUBSCRIBE,
	NONE,
	RESPONSE = 0x80,
	ANY = 0xFF
} Event;
#define REPLY(xxx) (Event)(xxx + Event::RESPONSE)
#define MAX_ACTORS 20

typedef uint8_t ActorRef;
class Header {
public:
	union {
		struct {
			uint8_t _dst;
			uint8_t _src;
			uint8_t _event;
			uint8_t _detail;
		};
		uint32_t _word;
	};
	Header() {
		_word = 0;
	}
	Header(ActorRef dst, ActorRef src, Event event, uint8_t detail) {
		_dst = dst;
		_src = src;
		_event = event;
		_detail = 0;
	}
	Header(int dst, int src, Event event, uint8_t detail); //
	Header(ActorRef src, Event event) :
			Header((ActorRef) 0, src, event, 0) {
	}
	Header(Event event) :
			Header((ActorRef) 0, (ActorRef) 0, event, 0) {
	}
	bool is(ActorRef dst, ActorRef src, Event event, uint8_t detail); //
	bool is(Event event,uint8_t detail);
	bool is(int dst, int src, Event event, uint8_t detail); //
	bool is(ActorRef dst, ActorRef src, uint8_t event, uint8_t detail); //
	inline bool is(uint8_t event) {
		return _event == event;
	}
	bool is(ActorRef src, Event event);
	Header& src(ActorRef);
	Header& dst(ActorRef);
	ActorRef src();
	ActorRef dst();
	Event event();

};

//#define LOGF(fmt,...) PrintHeader(__FILE__,__LINE__,__FUNCTION__);Serial.printf(fmt,##__VA_ARGS__);Serial.println();
//extern void PrintHeader(const char* file, uint32_t line, const char *function);

#define PT_BEGIN() bool ptYielded = true; switch (_ptLine) { case 0: // Declare start of protothread (use at start of Run() implementation).
#define PT_END() default: ; } ; return ; // Stop protothread and end it (use at end of Run() implementation).
// Cause protothread to wait until given condition is true.
#define PT_WAIT_UNTIL(condition) \
    do { _ptLine = __LINE__; case __LINE__: \
    if (!(condition)) return ; } while (0)

#define PT_WAIT_WHILE(condition) PT_WAIT_UNTIL(!(condition)) // Cause protothread to wait while given condition is true.
#define PT_WAIT_THREAD(child) PT_WAIT_WHILE((child).dispatch(msg)) // Cause protothread to wait until given child protothread completes.
// Restart and spawn given child protothread and wait until it completes.
#define PT_SPAWN(child) \
    do { (child).restart(); PT_WAIT_THREAD(child); } while (0)

// Restart protothread's execution at its PT_BEGIN.
#define PT_RESTART() do { restart(); return ; } while (0)

// Stop and exit from protothread.
#define PT_EXIT() do { stop(); return ; } while (0)

// Yield protothread till next call to its Run().
#define PT_YIELD() \
    do { ptYielded = false; _ptLine = __LINE__; case __LINE__: \
    if (!ptYielded) return ; } while (0)

// Yield protothread until given condition is true.
#define PT_YIELD_UNTIL(condition) \
    do { ptYielded = false; _ptLine = __LINE__; case __LINE__: \
    if (!ptYielded || !(condition)) return ; } while (0)
// Used to store a protothread's position (what Dunkels calls a
// "local continuation").
typedef unsigned short LineNumber;
// An invalid line number, used to mark the protothread has ended.
static const LineNumber LineNumberInvalid = (LineNumber) (-1);
// Stores the protothread's position (by storing the line number of
// the last PT_WAIT, which is then switched on at the next Run).

class Actor {
private:
	const char* _name;
	uint64_t _timeout;
	ActorRef _me;
	ActorRef _listener;
	static Actor* _actors[MAX_ACTORS];
	static uint32_t _count;
	static QueueTemplate<Header> _queue;
	uint32_t _state;

protected:
	LineNumber _ptLine;
	static const char* eventToString(uint8_t event);
	static Actor* _gDummy;
public:
	Actor(const char* name);
	virtual ~Actor();
	ActorRef me() {
		return _me;
	}
	ActorRef ref() {
		return _me;
	}
	virtual void on(Header hdr);
	virtual void setup() {
		on(Header(me(), INIT));
	}
	virtual void loop() {
		on(Header(me(), NONE));
	}
	void timeout(uint32_t time) {
		_timeout = millis() + time;
	}
	bool timeout() {
		return millis() > _timeout;
	}
	const char* name() {
		return _name;
	}
	inline void state(int st) {
		LOGF(" %s state change %d => %d", name(), _state, st);
		_state = st;
	}
	inline int state() {
		return _state;
	}
	inline ActorRef listener() {
		return _listener;
	}
	virtual inline void listener(ActorRef listener) {
		_listener = listener;
	}
	static void eventLoop();
	void publish(Event ev) ;
	static Actor& actor(ActorRef ref) {
		return *_actors[ref];
	}
	static void logHeader(const char* prefix,Header hdr);
};

/*

 Stm32::Stm32() {
	// TODO Auto-generated constructor stub

}

Stm32::~Stm32() {
	// TODO Auto-generated destructor stub
}

void Stm32::setup(TcpServer* tcp){
	_wifi=wifi;
}

Stm32::~Stm32() {
	// TODO Auto-generated destructor stub
}

void Stm32::on(Header hdr) {
	if (hdr.is(_wifi->ref(), REPLY(CONNECT))) {
		if (!MDNS.begin("esp8266")) {
			Serial.println("Error setting up MDNS responder!");
		}
	}
}

void Stm32::loop(){

}

 */


#endif /* ACTOR_H_ */
