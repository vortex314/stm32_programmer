#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <pins_arduino.h>
#include <Sys.h>
#include <Wifi.h>
#include <LedBlinker.h>
#include <WiFiUdp.h>
//#include <ESP8266mDNS.h>
#include <Stm32.h>
#include <mDNS.h>
#include <ctype.h>
#include <uart.h>
#include <Udp.h>
#include <ctype.h>
#include <cstdlib>
#include <Base64.h>
#include <Config.h>
#include <PubSubClient.h>

uint32_t BAUDRATE = 115200;
// Wifi wifi;
LedBlinker led;
Stm32 stm32;
mDNS mdns;
WiFiClient wifi_client;
IPAddress broker(37, 187, 106, 16);
PubSubClient client(wifi_client, "test.mosquitto.org");
String prefix = "wibo1/bootloader/";
String subscribe_topic = "put/" + prefix + "#";

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

#include <ArduinoJson.h>

void handle(JsonObject& resp, JsonObject& req);
void udpLog(char* str, uint32_t length);
/*
 #define UDP_MAX_SIZE	512

 class UdpServer: public Actor, public WiFiUDP {
 private:

 char buffer[UDP_MAX_SIZE];	//
 bool connected;

 public:
 static IPAddress _lastAddress;
 static uint16_t _lastPort;
 uint16_t _port;
 UdpServer() :
 Actor("UdpServer") {
 connected = false;
 _port = 1883;
 }

 void setConfig(uint16_t port) {
 _port = port;
 }

 void onWifiConnect(Header h) {
 begin(_port);
 Log.setOutput(udpLog);
 }

 bool isConnected() {
 return connected;
 }

 void loop() {
 int length;
 if (length = parsePacket()) {	// received data
 connected = true;
 if (length > UDP_MAX_SIZE) {
 LOGF(" UDP packet too big");
 return;
 }
 //			LOGF(" UDP rxd %s:%d in packet %d", remoteIP().toString().c_str(),
 //			 remotePort(), _packetIdx);
 _lastAddress = remoteIP();
 _lastPort = remotePort();

 //			LOGF("  %s ", remoteIP().toString().c_str());
 read(buffer, length); 			// read the packet into the buffer
 buffer[length] = '\0';
 StaticJsonBuffer<200> request;
 JsonObject& req = request.parseObject(buffer);
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
 }
 }
 };

 // UdpServer udp;
 IPAddress UdpServer::_lastAddress = IPAddress(192, 168, 0, 211);
 uint16_t UdpServer::_lastPort = 1883;*/

void udpLog(char* str, uint32_t length) {
	if (client.connected()) {
		str[length] = '\0';
		String message(str);
		client.publish(prefix + "system/log", message);
	}
	/*	if (udp.isConnected()) {
	 WiFiUDP Udp;
	 Udp.beginPacket(UdpServer::_lastAddress, UdpServer::_lastPort);
	 Udp.write(str, length);
	 Udp.write("\n");
	 Udp.endPacket();
	 }*/
}

void mqttPublish(const char* topic, String message) {
	if (client.connected())
		client.publish(prefix + topic, message);
}

class Publisher: public Actor {
	int _index;
public:
	Publisher() :
			Actor("Publisher") {
		_index = 0;
	}
	void init() {
		timeout(1000);
	}

	void onTimeout(Header hdr) {
		if (_index == 0) {
			mqttPublish("system/heap", String(ESP.getFreeHeap(), 10));
		} else if (_index == 1) {
			mqttPublish("system/esp_id", String(ESP.getChipId(), 16));
		} else if (_index == 2) {
			mqttPublish("system/uptime", String(millis(), 10));
		} else if (_index == 3) {
			mqttPublish("system/build", String(__DATE__ " " __TIME__));
		} else if (_index == 4) {
			mqttPublish("usart/rxd_count", String(Stm32::_usartRxd, 10));
		} else if (_index == 5) {
			mqttPublish("usart/baudrate", String(BAUDRATE, 10));
		} else if (_index == 6) {
			mqttPublish("stm32/mode",
					String(
							stm32.getMode() == Stm32::M_FLASH ?
									"APPLICATION" : "BOOTLOADER"));
		} else if (_index == 7) {
			mqttPublish("system/alive", "true");
		} else {
			_index = -1;
		}
		_index++;
		timeout(500);
	}

	void loop() {
		if (timeout())
			onTimeout(Header(Event::TIMEOUT));
	}
};

void handle(JsonObject& resp, JsonObject& req) {
	Serial.begin(BAUDRATE, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL);
	Serial.swap();
	String cmd = req["request"];
	resp["id"] = req["id"];
	resp["reply"] = req["request"];
	uint32_t startTime = millis();
	Erc erc = 0;
	if (cmd.equals("resetBootloader")) {

		erc = stm32.resetSystem();

	} else if (cmd.equals("resetFlash")) {

		erc = stm32.resetFlash();

	} else if (cmd.equals("goFlash")) {

		uint32_t address = req["address"];
		erc = stm32.go(address);
		resp["address"] = address;

	} else if (cmd.equals("status")) {

		resp["esp_id"] = ESP.getChipId();
		resp["heap"] = ESP.getFreeHeap();
		resp["upTime"] = millis();
		resp["version"] = __DATE__ " " __TIME__;
		resp["usart.rxd"] = Stm32::_usartRxd;
		resp["mode"] =
				stm32.getMode() == Stm32::M_FLASH ?
						"APPLICATION" : "BOOTLOADER";
		resp["baudrate"] = BAUDRATE;

	} else if (cmd.equals("getId")) {

		uint16_t chipId;
		erc = stm32.getId(chipId);
		if (erc == E_OK) {
			resp["chipId"] = chipId;
		}

	} else if (cmd.equals("getVersion")) {

		uint8_t version;
		erc = stm32.getVersion(version);
		if (erc == E_OK) {
			resp["version"] = version;
		}

	} else if (cmd.equals("get")) {

		Bytes data(30);
		Str strData(60);
		uint8_t version;
		erc = stm32.get(version, data);
		if (erc == E_OK) {
			erc = Base64::encode(strData, data);
			resp["cmds"] = String(strData.c_str());
			resp["version"] = version;
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

	} else if (cmd.equals("extendedEraseMemory")) {

		erc = stm32.extendedEraseMemory();

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

	} else if (cmd.equals("writeProtect")) {

		Bytes data(256);
		Str str((const char*) req["data"]);
		erc = Base64::decode(data, str);
		erc = stm32.writeProtect(data);

	} else if (cmd.equals("writeUnprotect")) {

		erc = stm32.writeUnprotect();

	} else if (cmd.equals("readoutProtect")) {

		erc = stm32.readoutProtect();

	} else if (cmd.equals("readoutUnprotect")) {

		erc = stm32.readoutUnprotect();

	} else if (cmd.equals("settings")) {

		if (req.containsKey("baudrate")) {
			BAUDRATE = req["baudrate"];
			resp["baudrate"] = BAUDRATE;
			Config.set("uart.baudrate", BAUDRATE);
		}
		String config;
		Config.load(config);
		resp["config"] = config;

	} else {
		erc = EINVAL;
	}
	resp["delta"] = millis() - startTime;
	resp["error"] = erc;

	Serial.begin(BAUDRATE, SerialConfig::SERIAL_8N1, SerialMode::SERIAL_FULL);
	Serial.swap();
}

// START Normal...

#define TIMEOUT_ENTER 50
#define ENTER 13            // byte value of the Enter key (LF)

void readLine(String& str, int timeout) {
	int index = 0;
	if (timeout == 0)
		timeout = 9999999;
	unsigned long end = millis() + timeout;

	while (true) {
		if (millis() > end) {
			Serial.println("\r\n TIMEOUT ! ");
			return;
		}
		if (Serial.available()) {

			char c = Serial.read();
			switch (c) {
			case '\n':
			case '\r':
				Serial.print("\r\n");
				return;
				break;

			case '\b':
				// backspace
				if (str.length() > 0) {
					str.remove(str.length() - 1);
					Serial.print(' ');
				}
				break;

			default:
				// normal character entered. add it to the buffer
				Serial.print(c);
				str.concat(c);
				break;
			}
		}
	}
}

void ask(String question, String& input, int timeout) {
	Serial.print(question);
	readLine(input, timeout);
}

void configMenu() {
	String configJson;
	Config.load(configJson);
	StaticJsonBuffer<400> jsonConf;
	JsonObject& object = jsonConf.parseObject(configJson);
	if (!object.success()) {
		LOGF(" parsing eeprom failed ");
	}
	String show;
	while (true) {
		show = "";
		object.prettyPrintTo(show);
//		object.printTo(show);
		Serial.println(
				"\r\n__________________ Config JSON ________________________________________");
		Serial.print(show);
		Serial.println(
				"\r\n_______________________________________________________________________");
		Serial.print("\r\n Create, Update, Delete, Save, eXit [cudsx] : ");
		String line;
		readLine(line, 0);
		Serial.println("");
		if (line.length() == 0)
			return;
		else if (line.equals("c") || line.equals("u")) {
			String key, value;
			ask("\r key   : ", key, 0);
			ask("\r value : ", value, 0);
			object[key] = value;
		} else if (line.equals("s")) {
			Serial.println("\r\n Saving.. ");
			show = "";
			object.printTo(show);
			Serial.println(show);
			Config.save(show);
			Serial.println("done.\r\n ");
		} else if (line.equals("d")) {
			String key;
			ask("\r key   : ", key, 0);
			object.remove(key);
		} else if (line.equals("x")) {
			Serial.println("\r\n Exit config");
			return;
		}
	}
}

void waitConfig() {
	String line;
	unsigned long end = millis() + 5000;
	while (true) {
		if (millis() > end) {
			Serial.println(" input timed out, starting...");
			break;
		}
		Serial.printf(" Hit 'x' + Enter for menu within 5 seconds : ");
		readLine(line, 10000);
		if (line.equals("x")) {
			configMenu();
			return;
		} else {
			Serial.printf(" invalid input >>%s<< , try again.\n", line.c_str());
		}
		line = "";
	}
}

//________________________________________________Se_________________
#ifndef WIFI_SSID
#define WIFI_SSID "YourNetworkSSID"
#endif
#ifndef WIFI_PSWD
#define WIFI_PSWD "YourNetworkPassword"
#endif

void loadConfig() {
	String ssid, password, hostname, service, mqttHost;
	uint32_t mqttPort;
	char hostString[16] = { 0 };
	sprintf(hostString, "ESP_%06X", ESP.getChipId());

	waitConfig();

	Config.get("uart.baudrate", BAUDRATE, 115200);
	Config.get("wifi.ssid", ssid, WIFI_SSID);
	Config.get("wifi.pswd", password, WIFI_PSWD);
	Config.get("wifi.hostname", hostname, hostString);
	Config.get("mqtt.port", mqttPort, 1883);
	Config.get("mqtt.host", mqttHost, "test.mosquitto.org");
	Config.get("mqtt.prefix", prefix, "wibo1/bootloader/");
	subscribe_topic = "put/" + prefix + "#";

//TODO	wifi.setConfig(ssid, password, hostname);
//	udp.setConfig(udpPort);
//	mdns.setConfig(service, udpPort);
	client.set_server(mqttHost, mqttPort);

}

Publisher publisher;

extern "C" void setup() {
	Serial.begin(BAUDRATE, SerialConfig::SERIAL_8N1, SerialMode::SERIAL_FULL);
	Serial.setDebugOutput(false);
	loadConfig();

	LOGF(" version : " __DATE__ " " __TIME__);

	//TODO	LOGF(" starting Wifi host : '%s' on SSID : '%s' '%s' ", wifi.getHostname(),
	//TODO			wifi.getSSID(), wifi.getPassword());
	delay(100);	// flush serial delay

	publisher.on(TIMEOUT, publisher, (EventHandler) &Publisher::onTimeout);

	//TODO	wifi.on(CONNECT, led, (EventHandler) &LedBlinker::blinkFast);
	//TODO	wifi.on(DISCONNECT, led, (EventHandler) &LedBlinker::blinkSlow);
	//TODO	wifi.on(CONNECT, mdns, (EventHandler) &mDNS::onWifiConnected);
	//TODO	wifi.on(CONNECT, udp, (EventHandler) &UdpServer::onWifiConnect);

	Actor::initAll();
	return;
}

#define BUFFER_SIZE 100

void callback(const MQTT::Publish& pub) {
	if (pub.topic().endsWith("request")) {
		String sRequest = pub.payload_string();
		StaticJsonBuffer<1000> request;
		JsonObject& req = request.parseObject(sRequest);
		StaticJsonBuffer<1000> reply;
		JsonObject& repl = reply.createObject();
		handle(repl, req);
		if (!req.success()) {
			LOGF(" UDP message JSON parsing fails");
			client.publish("system/log", " UDP message JSON parsing fails");
			return;
		} else {
			handle(repl, req);
			String str;
			repl.printTo(str);
			client.publish(prefix + "bootloader/reply", str);
		}
	} else {
		LOGF(" unknown request %s", pub.payload_string().c_str());
	}
}

extern "C" void loop() {
	Actor::eventLoop();
	if (WiFi.status() != WL_CONNECTED) {
		Serial.print("Connecting to ");
		Serial.print(WIFI_SSID);
		Serial.println("...");
		WiFi.begin(WIFI_SSID, WIFI_PSWD);

		if (WiFi.waitForConnectResult() != WL_CONNECTED) {
			LOGF(" still connecting ");
			return;
		}
		LOGF("WiFi connected");
	}

	if (WiFi.status() == WL_CONNECTED) {
//	if (wifi.connected()) {
		if (!client.connected()) {
			if (client.connect("wibo1", prefix + "system/alive", 1, 0,
					"false")) {
				client.publish(prefix + "system/alive", "true");
				client.set_callback(callback);
				client.subscribe(subscribe_topic);
				Log.setOutput(udpLog);
				LOGF(" mqtt client connected ");
			} else {
				LOGF(" mqtt connect failed ")
			}
		}
		if (client.connected())
			client.loop();
	}
}

