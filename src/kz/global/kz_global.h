#pragma once
#include "../kz.h"

#include <vendor/ixwebsocket/ixwebsocket/IXWebSocket.h>

class KZGlobalService : public KZBaseService
{
	using KZBaseService::KZBaseService;

public:
	static void Init();

	/**
	 * Returns wheter we are currently connected to the API.
	 */
	static bool Connected()
	{
		// 0 is a dummy value we use if we aren't connected
		return heartbeatInterval != 0.0;
	}

private:
	/**
	 * API URL to use for making requests.
	 */
	static std::string apiUrl;

	/**
	 * Key to use for authenticating with the API.
	 */
	static std::string apiKey;

	/**
	 * The WebSocket we use to communicate with the API.
	 */
	static ix::WebSocket *apiSocket;

	/**
	 * Interval at which we need to send heartbeat messages.
	 */
	static f64 heartbeatInterval;

	/**
	 * Monotonically incrementing integer we use to tie requests and responses
	 * together.
	 */
	static u64 currentMessageId;

	/**
	 * Callback invoked by the WebSocket library anytime we receive a message.
	 */
	static void OnMessageCallback(const ix::WebSocketMessagePtr &message);

	/**
	 * Callback invoked right after the WebSocket connection has been established.
	 */
	static void PerformHandshake(const ix::WebSocketMessagePtr &message);

	/**
	 * Gets called every `heartbeatInterval` seconds and sends a heartbeat
	 * message to the API.
	 */
	static f64 Heartbeat();
};
