#pragma once

#include <optional>

#include "../kz.h"
#include "utils/json.h"
#include "message.h"
#include "map.h"

#include <vendor/ixwebsocket/ixwebsocket/IXWebSocket.h>

class KZGlobalService : public KZBaseService
{
	using KZBaseService::KZBaseService;

public:
	static void Init();
	static void OnActivateServer();

	/**
	 * Returns wheter we are currently connected to the API.
	 */
	static bool Connected()
	{
		return apiSocket && apiSocket->getReadyState() == ix::ReadyState::Open;
	}

	/**
	 * Gets called every `heartbeatInterval` seconds and sends a heartbeat
	 * message to the API.
	 */
	static f64 Heartbeat();

	/**
	 * Fetches a map by its ID.
	 *
	 * Note that this must be the ID used by the API, not the local database.
	 */
	static void FetchMap(u16 mapId, std::function<void(std::optional<KZ::API::Map>)> callback);

	/**
	 * Fetches a map by its name.
	 */
	static void FetchMap(const char *mapName, std::function<void(std::optional<KZ::API::Map>)> callback);

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
	static u64 nextMessageId;

	struct Callback
	{
		u64 messageId;
		std::function<void(json)> callback;
	};

	/**
	 * Callbacks for 'requests'.
	 *
	 * Most of the public functions in this class send messages to the API that
	 * act as requests. The API is going to respond to these requests with
	 * messages containing the same ID as the message originally sent by us.
	 * So, whenever we make a request, we push a callback into this vector
	 * containing the message ID and a function we want to call when the API
	 * responds.
	 */
	static std::vector<Callback> callbacks;

	/**
	 * Sends a message over the WebSocket.
	 */
	template<typename T>
	static void SendMessage(KZ::API::Message<T> message, std::function<void(json)> callback);

	/**
	 * Callback invoked by the WebSocket library anytime we receive a message.
	 */
	static void OnMessageCallback(const ix::WebSocketMessagePtr &message);

	/**
	 * Callback invoked right after the WebSocket connection has been established.
	 */
	static void PerformHandshake(const ix::WebSocketMessagePtr &message);
};
