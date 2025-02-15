#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <string_view>

#include <vendor/ixwebsocket/ixwebsocket/IXWebSocket.h>

#include "utils/json.h"

#include "kz/kz.h"

class KZGlobalService : public KZBaseService
{
	using KZBaseService::KZBaseService;

public:
	static bool IsAvailable();

	/**
	 * Executes a function with information about the current map.
	 *
	 * `F` should be a function that accepts a single argument of type `const KZ::API::Map*`.
	 */
	template<typename F>
	static auto WithCurrentMap(F &&f) const
	{
		std::unique_lock lock(KZGlobalService::currentMap.mutex);
		const KZ::API::Map *currentMap = nullptr;

		if (KZGlobalService::currentMap.data.has_value())
		{
			currentMap = &*KZGlobalService::currentMap.data;
		}

		return f(currentMap);
	}

	static void UpdateRecordCache();

public:
	static void Init();
	static void Cleanup();
	static void RegisterCommands();

	static void OnServerGamePostSimulate();
	static void OnActivateServer();
	static void OnPlayerAuthorized();
	static void OnClientDisconnect();

private:
	enum class State
	{
		/**
		 * The default state
		 */
		Uninitialized,

		/**
		 * After `Init()` has returned
		 */
		Initialized,

		/**
		 * After we receive an "open" message on the WS thread
		 */
		Connected,

		/**
		 * After we sent the 'Hello' message
		 */
		HandshakeInitiated,

		/**
		 * After we received the 'HelloAck' message
		 */
		HandshakeCompleted,

		/**
		 * After the server closed the connection for a reason that won't change
		 */
		Disconnected,

		/**
		 * After the server closed the connection, but it's still worth to retry the connection
		 */
		DisconnectedButWorthRetrying,
	};

	static inline std::atomic<State> state = State::Uninitialized;

	static inline struct
	{
		std::mutex mutex;
		std::vector<std::function<void()>> queue;
	} mainThreadCallbacks {};

	// invariant: should be `nullptr` if `state == Uninitialized` and otherwise a valid pointer
	static inline ix::WebSocket *socket = nullptr;
	static inline std::atomic<u32> nextMessageID = 1;

	static inline struct
	{
		std::mutex mutex;
		std::unordered_map<u32, std::function<void(u32, const Json &)>> queue;
	} messageCallbacks {};

	static inline struct
	{
		std::mutex mutex;
		std::optional<KZ::API::Map> data;
	} currentMap {};

	static void EnforceConVars();
	static void RestoreConVars();

	template<typename CB>
	static void AddMainThreadCallback(CB &&callback)
	{
		std::unique_lock lock(KZGlobalService::mainThreadCallbacks.mutex);
		KZGlobalService::mainThreadCallbacks.queue.emplace_back(std::move(callback));
	}

	template<typename CB>
	static void AddMessageCallback(u32 messageID, CB &&callback)
	{
		std::unique_lock lock(KZGlobalService::messageCallbacks.mutex);
		KZGlobalService::messageCallbacks.queue[messageID] = std::move(callback);
	}

	static void ExecuteMessageCallback(u32 messageID, const Json &payload);

	template<typename T>
	static bool PrepareMessage(std::string_view event, u32 messageID, const T &data, Json &payload)
	{
		if (*KZGlobalService::state != State::Connected)
		{
			META_CONPRINTF("[KZ::Global] WARN: called `SendMessage()` before connection was established\n");
			return false;
		}

		if (!payload.Set("id", messageID))
		{
			return false;
		}

		if (!payload.Set("event", event))
		{
			return false;
		}

		if (!payload.Set("data", data))
		{
			return false;
		}

		return true;
	}

	template<typename T>
	static bool SendMessage(std::string_view event, const T &data)
	{
		Json payload;

		if (!KZGlobalService::PrepareMessage(event, KZGlobalService::nextMessageID++, data, payload))
		{
			return false;
		}

		KZGlobalService::socket->send(payload.ToString());
	}

	template<typename T, typename CB>
	static bool SendMessage(std::string_view event, const T &data, CB &&callback)
	{
		u32 messageID = KZGlobalService::nextMessageID++;
		Json payload;

		if (!KZGlobalService::PrepareMessage(event, messageID, data, payload))
		{
			return false;
		}

		// clang-format off
		KZGlobalService::AddMessageCallback(messageID, [callback = std::move(callback)](u32 messageID, const Json& payload)
		{
			if (!responseJson.IsValid())
			{
				META_CONPRINTF("[KZ::Global] WebSocket message is not valid JSON.\n");
				return;
			}

			auto decoded;

			if (!responseJson.Get("data", decoded))
			{
				META_CONPRINTF("[KZ::Global] WebSocket message does not contain a valid `data` field.\n");
				return;
			}

			callback(std::move(decoded));
		});
		// clang-format on

		KZGlobalService::socket->send(payload.ToString());
	}
};
