#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <pins_arduino.h>
#include <Sys.h>
#include <Wifi.h>
#include <LedBlinker.h>
#include <TcpServer.h>
#include <TcpClient.h>
#include <WiFiUdp.h>

#include <ESP8266mDNS.h>
#include <Stm32.h>
#include <mDNS.h>
#include <ctype.h>
#include <uart.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Udp.h>
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

unsigned long strtoul(const char *nptr, char **endptr, register int base) {
	return strtol(nptr, endptr, base);
}

//________________________________________________Se_________________
Wifi wifi("Merckx", "LievenMarletteEwoutRonald");
LedBlinker led;
// TcpServer tcpServer(23);
// UdpServer udpServer(3881);
//WifiWebServer webServer(80);

Stm32 stm32;
mDNS mdns;

#include <Base64.h>
#include <ArduinoJson.h>
//#include <MQTT.h>
#include <PubSubClient.h>

Bytes bytesIn(450);
Bytes bytesOut(450);
Str strOut(450);

void handle(JsonObject& resp, JsonObject& req) {
	String cmd = req["request"];
	resp["id"] = req["id"];
	resp["reply"] = req["request"];
	uint32_t startTime = millis();
	Erc erc = 0;
	if (cmd.equals("reset")) {
		erc = stm32.reset();
	} else if (cmd.equals("status")) {
		resp["esp_id"] = ESP.getChipId();
		resp["freq_Mhz"] = ESP.getCpuFreqMHz();
		resp["flash_size"] = ESP.getFlashChipSize();
		resp["heap"] = ESP.getFreeHeap();
		resp["Vcc"] = ESP.getVcc();
		resp["upTime"] = millis();
	} else if (cmd.equals("getId")) {
		uint16_t chipId;
		erc = stm32.getId(chipId);
		if (erc == E_OK) {
			resp["chipId"] = chipId;
		}
	} else if (cmd.equals("get")) {

		Bytes data(30);
		Str strData(60);
		erc = stm32.get(data);
		if (erc == E_OK) {
			erc = Base64::encode(strData, data);
			resp["cmds"] = String(strData.c_str());
		}
	} else if (cmd.equals("writeMemory")) {

		Bytes data(256);
		Str str((const char*) req["data"]);
		uint32_t address = req["address"];
		resp["address"] = address;
		erc = Base64::decode(data, str);
		resp["length"] = data.length();
		if (erc == E_OK) {
			erc = stm32.writeMemory(address, data);
		}
	} else if (cmd.equals("eraseMemory")) {

		Bytes pages(256);
		Str str((const char*) req["pages"]);
		erc = Base64::decode(pages, str);
		resp["length"] = pages.length();
		erc = stm32.eraseMemory(pages);
	} else if (cmd.equals("eraseAll")) {

		erc = stm32.eraseAll();
	} else if (cmd.equals("readMemory")) {

		Str strData(410);
		Bytes data(256);
		uint32_t address = req["address"];
		uint32_t length = req["length"];
		resp["address"] = address;
		resp["length"] = length;
		erc = stm32.readMemory(address, length, data);
		if (erc == E_OK) {
			erc = Base64::encode(strData, data);
			resp["data"] = String(strData.c_str());
		}
	}  else if (cmd.equals("go")) {
		uint32_t address = req["address"];
		resp["address"] = address;
		erc = stm32.go(address);
	} else {
		erc = EINVAL;
	}
	resp["delta"] = millis() - startTime;
	resp["error"] = erc;

}
/*
 void onMessageReceived(String topic, String message) {
 //	LOGF(" recv %s : %s \n", topic.c_str(), message.c_str());

 const int SIZE = JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(1);
 StaticJsonBuffer<1000> request;
 StaticJsonBuffer<1000> reply;
 JsonObject& req = request.parseObject(message);
 JsonObject& repl = reply.createObject();
 if (!req.success()) {
 LOGF(" parsing request failed ");
 } else {
 handle(repl, req);
 String str;
 repl.printTo(str);

 }
 }*/

void udpLog(char* str, uint32_t length);

#define UDP_MAX_SIZE	512
#define UDP_PACKETS	3
class UdpServer: public Actor, public WiFiUDP {
private:
	typedef struct {
		IPAddress ip;
		uint16_t port;
		char buffer[UDP_MAX_SIZE];	//
		bool used;
	} Packet;
	Packet _packet[UDP_PACKETS];
	int _packetIdx;
	Packet* _current;

public:
	static IPAddress _lastAddress;
	static uint16_t _lastPort;
	UdpServer() :
			Actor("UdpServer") {
		_packetIdx = 0;
		_packet[_packetIdx].used = false;
		_current = &_packet[0];
	}

	void onWifiConnect(Header h) {
		begin(1883);
		Log.setOutput(udpLog);
	}

	void loop() {
		int length;
		if (length = parsePacket()) {	// received data
			if (length > UDP_MAX_SIZE) {
				LOGF(" UDP packet too big");
				return;
			}
			if (_packet[_packetIdx].used) {
				LOGF(" UDP buffer overflow");
				return;
			}
			/*		LOGF(" UDP rxd %s:%d in packet %d", remoteIP().toString().c_str(),
			 remotePort(), _packetIdx);*/
			_lastAddress = remoteIP();
			_lastPort = remotePort();
			_current = &_packet[_packetIdx];
			read(_current->buffer, length); // read the packet into the buffer
			_current->buffer[length] = '\0';
			StaticJsonBuffer<200> request;
			JsonObject& req = request.parseObject(_current->buffer);
			StaticJsonBuffer<1000> reply;
			JsonObject& repl = reply.createObject();
			if (!req.success()) {
				LOGF(" UDP message JSON parsing fails");
				return;
			} else {
				handle(repl, req);
				String str;
				repl.printTo(str);
				beginPacket(remoteIP(), remotePort());
				write(str.c_str(), str.length());
				endPacket();
			}
			publish(RXD);
			_packetIdx = ++_packetIdx % UDP_PACKETS;
		}
	}
};

UdpServer udp;
IPAddress UdpServer::_lastAddress=IPAddress(192,168,0,210);
uint16_t UdpServer::_lastPort=3881;

void udpLog(char* str, uint32_t length) {
	WiFiUDP Udp;
	Udp.beginPacket(UdpServer::_lastAddress, UdpServer::_lastPort);
	Udp.write(str, length);
	Udp.endPacket();
}

extern "C" void setup() {
	Serial.begin(115200, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL);
	Serial1.begin(115200, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_TX_ONLY);
	led.init();
	LOGF(" starting .... ");
	delay(100);
	stm32.begin();

	wifi.on(CONNECT, led, (EventHandler) &LedBlinker::blinkFast);
	wifi.on(DISCONNECT, led, (EventHandler) &LedBlinker::blinkSlow);
	wifi.on(CONNECT, mdns, (EventHandler) &mDNS::onWifiConnected);
	wifi.on(CONNECT, udp, (EventHandler) &UdpServer::onWifiConnect);
//	udp.on(RXD,stm32,(EventHandler) &Stm32::onWifiConnect);

	Actor::initAll();
	return;
}


extern "C" void loop() {
	Actor::eventLoop();
}

