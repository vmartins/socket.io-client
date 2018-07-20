#include <SocketIoClient.h>
#include <ArduinoJson.h>

const String getEventName(const String msg) {
	DynamicJsonBuffer jsonBuffer(msg.length());
	JsonArray& jsonArray = jsonBuffer.parseArray(msg.substring(msg.indexOf("[")));
	String name = jsonArray[0];

	return name;
}

const JsonVariant getEventPayload(const String msg) {
	DynamicJsonBuffer jsonBuffer(msg.length());
	JsonArray& jsonArray = jsonBuffer.parseArray(msg.substring(msg.indexOf("[")));
	JsonVariant payload = jsonArray[1];

	return payload;
}

void SocketIoClient::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
	String msg;

	switch(type) {
		case WStype_DISCONNECTED:
			SOCKETIOCLIENT_DEBUG("[SIoC] Disconnected!\n");
            trigger("disconnected", NULL);
			break;
		case WStype_CONNECTED:
			SOCKETIOCLIENT_DEBUG("[SIoC] Connected to url: %s\n", payload);
			_webSocket.sendTXT("5");
			break;
		case WStype_TEXT:
			msg = String((char*)payload);

			if(msg.startsWith("42")) {
				trigger(getEventName(msg).c_str(), getEventPayload(msg));
			} else if(msg.startsWith("2")) {
				_webSocket.sendTXT("3");
			} else if(msg.startsWith("40")) {
				trigger("connect", NULL);
			} else if(msg.startsWith("0")) {
				DynamicJsonBuffer jsonBuffer(length);
				JsonObject& jsonObject = jsonBuffer.parseObject(msg.substring(1,msg.length()));
				if (jsonObject.success()) {
				   	_pingInterval = jsonObject["pingInterval"];
					SOCKETIOCLIENT_DEBUG("[SIoC] pingInterval: %d\n",  _pingInterval);
			   	} else {
					SOCKETIOCLIENT_DEBUG("[SIoC] JSON parse failed: %s\n", msg.substring(1,msg.length()).c_str());
			   	}
			}
			break;
		case WStype_BIN:
			SOCKETIOCLIENT_DEBUG("[SIoC] get binary length: %u\n", length);
			hexdump(payload, length);
		break;
	}
}

void SocketIoClient::beginSSL(const char* host, const int port, const char* url, const char* fingerprint) {
	_webSocket.beginSSL(host, port, url, fingerprint); 
    initialize();
}

void SocketIoClient::begin(const char* host, const int port, const char* url) {
	_webSocket.begin(host, port, url);
    initialize();
}

void SocketIoClient::initialize() {
    _webSocket.onEvent(std::bind(&SocketIoClient::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	_lastPing = millis();
}

void SocketIoClient::loop() {
	_webSocket.loop();
	for(auto packet=_packets.begin(); packet != _packets.end();) {
		if(_webSocket.sendTXT(*packet)) {
			SOCKETIOCLIENT_DEBUG("[SIoC] packet \"%s\" emitted\n", packet->c_str());
			packet = _packets.erase(packet);
		} else {
			++packet;
		}
	}

	if(millis() - _lastPing > _pingInterval) {
		_webSocket.sendTXT("2");
		_lastPing = millis();
	}
}

void SocketIoClient::on(const char* event, std::function<void (const JsonVariant payload)> func) {
	_events[event] = func;
}

void SocketIoClient::emit(const char* event, JsonVariant payload) {
	DynamicJsonBuffer jsonBuffer;
	JsonArray& array = jsonBuffer.createArray();
	array.add(event);
	array.add(payload);
	String arrayString;
	array.printTo(arrayString);
	String msg = String((char*)"42") + arrayString;

	SOCKETIOCLIENT_DEBUG("[SIoC] add packet %s\n", msg.c_str());
	_packets.push_back(msg);
}

void SocketIoClient::trigger(const char* event, const JsonVariant payload) {
	auto e = _events.find(event);
	if(e != _events.end()) {
		SOCKETIOCLIENT_DEBUG("[SIoC] trigger event %s\n", event);
		e->second(payload);
	} else {
		SOCKETIOCLIENT_DEBUG("[SIoC] event %s not found. %d events available\n", event, _events.size());
	}
}