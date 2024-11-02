#pragma once

#include <optional>

#include "../kz.h"
#include "utils/json.h"
#include "message.h"
#include "map.h"
#include "game_session.h"

#include <vendor/ixwebsocket/ixwebsocket/IXWebSocket.h>

class KZGlobalService : public KZBaseService
{
	using KZBaseService::KZBaseService;

public:
	static void Init();
	static void RegisterCommands();

	/**
	 * Called whenever the server loads a new map.
	 *
	 * This also takes care of the `MapChange` API event; there is no separate
	 * method for that.
	 */
	static void OnActivateServer();

	/**
	 * Called whenever the player joins a team.
	 */
	void OnPlayerJoinTeam(i32 team);

	/**
	 * Called whenever the player enters an end zone.
	 */
	void OnTimerEnd(u32 announceID, u32 globalCourseID, KZ::API::Mode mode, u32 styles, u32 teleports, f64 time, u64 playerID);

	/**
	 * Returns wheter we are currently connected to the API.
	 */
	static bool Connected()
	{
		return KZGlobalService::apiSocket && KZGlobalService::apiSocket->getReadyState() == ix::ReadyState::Open;
	}

	/**
	 * Called whenever the player count changes.
	 */
	static void PlayerCountChange(KZPlayer *currentPlayer = nullptr);

	/**
	 * Called when a player disconnects.
	 */
	void PlayerDisconnect();

	/**
	 * Fetches a map by its name.
	 */
	static void FetchMap(const char *mapName, std::function<void(std::optional<KZ::API::Map>)> callback);

	/**
	 * Fetches the player's preferences from the API and registers them in the
	 * option service.
	 */
	void InitializePreferences();

	/**
	 * Called when the server shuts down.
	 *
	 * This closes the WS connection gracefully.
	 */
	static void Cleanup();

	/**
	 * API information about the current map.
	 *
	 * Will be `std::nullopt` if the current map is not global.
	 */
	static std::optional<KZ::API::Map> currentMap;

	/**
	 * The player's current session.
	 */
	KZ::API::GameSession session {};

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
	 * Interval at which we need to send heartbeat pings.
	 */
	static f64 heartbeatInterval;

	/**
	 * Monotonically incrementing integer we use to tie requests and responses
	 * together.
	 */
	static u64 nextMessageId;

	/**
	 * A callback to execute when a message with a specific message ID arrives.
	 */
	struct Callback
	{
		/**
		 * The message ID we expect from the response.
		 */
		u64 messageId;

		/**
		 * The actual callback to execute.
		 */
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
	 * Gets called every `heartbeatInterval` seconds and sends a heartbeat
	 * message to the API.
	 */
	static f64 Heartbeat();

	/**
	 * The function executed by the heartbeat thread.
	 *
	 * It will call `KZGlobalService::Heartbeat()` repeatedly, sleeping after
	 * every call for the amount of seconds returned by the call.
	 *
	 * If `KZGlobalService::Heartbeat()` ever returns 0, or a negative value,
	 * the function will return and the thread will exit.
	 */
	static void HeartbeatThread();

	/**
	 * Callback invoked by the WebSocket library anytime we receive a message.
	 */
	static void OnMessageCallback(const ix::WebSocketMessagePtr &message);

	/**
	 * Callback invoked right after the WebSocket connection has been
	 * established.
	 */
	static void PerformHandshake(const ix::WebSocketMessagePtr &message);

	/**
	 * Sends a message over the WebSocket.
	 */
	template<typename T>
	static void SendMessage(KZ::API::Message<T> message);

	/**
	 * Sends a message over the WebSocket with a callback to execute when we
	 * get a response.
	 */
	template<typename T>
	static void SendMessage(KZ::API::Message<T> message, std::function<void(json)> callback);
};

namespace KZ::global
{
	u32 styleNamesToBitflags(const CCopyableUtlVector<CUtlString> &styleNames);
}; // namespace KZ::global
