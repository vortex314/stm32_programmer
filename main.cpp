#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <pins_arduino.h>
#include <Sys.h>
#include <Wifi.h>
#include <LedBlinker.h>
#include <TcpServer.h>
#include <TcpClient.h>

#include <ESP8266mDNS.h>
#include <Stm32.h>
#include <mDNS.h>
#include <ctype.h>
#include <uart.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
//#include <WifiWebServer.h>

int islower(int a) {
	return (a >= 'a' && a <= 'z');
}

int tolower(int a) {
	if (islower(a))
		return a;
	return a - 'A' + 'a';
}

int isupper(int c) {
	return (c >= 'A' && c <= 'Z');
}

int isalpha(int c) {
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int isspace(int c) {
	return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

inline int isdigit(char c) {
	return (c >= '0' && c <= '9');
}

#include <limits.h>

/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long strtol(const char *nptr, char **endptr, register int base) {
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	} else if ((base == 0 || base == 2) && c == '0'
			&& (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		base = 2;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long) LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long) base;
	cutoff /= (unsigned long) base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
//		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

char* strchr(const char* str, int l) {
	char* ptr = (char*) str;
	while (*ptr != 0) {
		if (*ptr == (char) l)
			return ptr;
		ptr++;
	}
	return 0;
}

//_________________________________________________________________
Wifi wifi("Merckx", "LievenMarletteEwoutRonald");
LedBlinker led;
// TcpServer tcpServer(23);
// UdpServer udpServer(3881);
//WifiWebServer webServer(80);

Stm32 stm32;
mDNS mdns;

#include <Base64.h>
#include <ArduinoJson.h>
#include <MQTT.h>
#include <PubSubClient.h>

void mqttLog(char* s, uint32_t length);

class Mqtt: public Actor, public PubSubClient {
private:
	WiFiClient _wclient;
	long lastReconnectAttempt;
	enum {
		DISCONNECTED, CONNECTED
	} _state;
public:
	Mqtt() :
			Actor("Mqtt"), PubSubClient(_wclient, "iot.eclipse.org") {
		_state = DISCONNECTED;
		lastReconnectAttempt = 0;
	}
	void init() {
		timeout(100);
	}

	void publishTopic(String topic, String message) {
		if (connected())
			PubSubClient::publish(topic, message);
	}

	bool reconnect() {
		String clientId = "ESP8266_" + millis();
		if (connect(clientId, "stm32/alive", 1, false, "false")) {
			Actor::publish(CONNECT);
			publishTopic("stm32/alive", "true");
			subscribe("stm32/in/#");
		}
		Log.setOutput(mqttLog);
		return connected();
	}

	void onWifiConnect(Header hdr) {
		if (!connected()) {
			reconnect();
		}
	}
	void state(uint32_t st) {
		if (st != Actor::state()) {
			Actor::state(st);
			if (st == CONNECTED) {
				LOGF(" Mqtt Connected.");
				Actor::publish(CONNECT);
			} else {
				LOGF(" Mqtt Disconnected.");
				Actor::publish(DISCONNECT);
			}
		}
	}
	void loop() {
		if (timeout()) {
//			LOGF("timeout -  connected %d",connected());
			String str(millis());
			publishTopic("stm32/log", str);
			publishTopic("stm32/heap", String(ESP.getFreeHeap()));
			timeout(1000);
		}
		state(connected() ? CONNECTED : DISCONNECTED);
		if (!connected()) {
			long now = millis();
			if (now - lastReconnectAttempt > 5000) {
				lastReconnectAttempt = now;
				// Attempt to reconnect
				if (reconnect()) {
					lastReconnectAttempt = 0;
				}
			}
		} else {
			// Client connected

			PubSubClient::loop();
		}
	}
};

Mqtt mqtt;

Bytes bytesIn(300);
Bytes bytesOut(300);
Str strOut(400);

void handle(JsonObject& resp, JsonObject& req) {
	String dataIn = req["data"];
	Str strIn(dataIn.c_str());
	Erc erc = Base64::decode(bytesIn, strIn);	//
	LOGF(" length : %d ", bytesIn.length());
	resp["error"] = Stm32::engine(bytesOut, bytesIn);
	erc = Base64::encode(strOut, bytesOut);
	resp["data"] = String(strOut.c_str());
	resp["id"] = req["id"];
	resp["cmd"] = req["cmd"];
	resp["time"] = millis();
}

void onMessageReceived(String topic, String message) {
	LOGF(" recv %s : %s \n", topic.c_str(), message.c_str());

	if (topic == "stm32/in/request") {
		StaticJsonBuffer<200> request;
		StaticJsonBuffer<200> reply;
		JsonObject& req = request.parseObject(message);
		JsonObject& repl = reply.createObject();
		handle(repl, req);
		String str;
		repl.printTo(str);
		mqtt.publishTopic("stm32/reply", str);
		LOGF(" out stm32/reply : %s\n", str.c_str());
	} else {
		LOGF(" unknown topic received %s ", topic.c_str());
	}

}

void callback(const MQTT::Publish& pub) {
	LOGF(" published : %s : %s ", pub.topic().c_str(),
			pub.payload_string().c_str());
	onMessageReceived(pub.topic(), pub.payload_string());
}

void mqttLog(char* s, uint32_t length) {
	if (mqtt.connected())
		mqtt.publishTopic("stm32/log", s);
}

extern "C" void setup() {
	Serial.begin(115200, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL);
	Serial1.begin(115200, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_TX_ONLY);
	led.init();
	stm32.begin();

	mqtt.on(CONNECT, led, (EventHandler) &LedBlinker::blinkFast);
	mqtt.on(DISCONNECT, led, (EventHandler) &LedBlinker::blinkSlow);
	wifi.on(CONNECT, mdns, (EventHandler) &mDNS::onWifiConnected);
	wifi.on(CONNECT, mqtt, (EventHandler) &Mqtt::onWifiConnect);

	mqtt.set_callback(callback);
	Actor::initAll();
	return;
}

extern "C" void loop() {
	Actor::eventLoop();
}

