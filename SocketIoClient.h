#ifndef __SOCKET_IO_CLIENT_H__
#define __SOCKET_IO_CLIENT_H__

#include <Arduino.h>
#include <map>
#include <vector>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define SOCKETIOCLIENT_DEBUG(...) Serial.printf(__VA_ARGS__);
//#define SOCKETIOCLIENT_DEBUG(...)

//#define SOCKETIOCLIENT_USE_SSL
#ifdef SOCKETIOCLIENT_USE_SSL
	#define DEFAULT_PORT 443
#else
	#define DEFAULT_PORT 80
#endif
#define DEFAULT_URL "/socket.io/?transport=websocket"
#define DEFAULT_FINGERPRINT ""


class SocketIoClient {
private:
	std::vector<String> _packets;
	WebSocketsClient _webSocket;
	int _lastPing;
    int _pingInterval;
	std::map<String, std::function<void (const JsonVariant payload)>> _events;

	void trigger(const char* event, const JsonVariant payload);
	void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    void initialize();
public:
    void beginSSL(const char* host, const int port = DEFAULT_PORT, const char* url = DEFAULT_URL, const char* fingerprint = DEFAULT_FINGERPRINT);
	void begin(const char* host, const int port = DEFAULT_PORT, const char* url = DEFAULT_URL);
	void loop();
	void on(const char* event, std::function<void (const JsonVariant payload)>);
	void emit(const char* event, JsonVariant payload);
};

#endif