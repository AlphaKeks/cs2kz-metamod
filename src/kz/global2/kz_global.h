#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <vendor/ixwebsocket/ixwebsocket/IXWebSocket.h>

#include "common.h"
#include "cs2kz.h"
#include "kz/kz.h"
#include "utils/json.h"
#include "messages.h"

class KZGlobalService2 : public KZBaseService
{
	using KZBaseService::KZBaseService;

	enum class State
	{
		/**
		 * The state before `Init()` is called and after `Cleanup()` has been called.
		 */
		Uninitialized,

		/**
		 * The state after `Init()` has been called, when the WebSocket has been configured, but not yet started.
		 */
		Configured,

		/**
		 * The state after `WS::socket->start()` has been called.
		 */
		Connecting,

		/**
		 * The state after the WebSocket connection has been fully established.
		 */
		Connected,

		/**
		 * The state after we sent our `hello` message to the API.
		 */
		HandshakeInitiated,

		/**
		 * The state after we received a `hello_ack` response to our `hello` message from the API.
		 *
		 * This is the final "in operation" state when the connection can actually be "used" for normal message exchange.
		 */
		HandshakeCompleted,

		/**
		 * The state after we've been disconnected but haven't yet reconnected.
		 */
		Reconnecting,

		/**
		 * The state when we're not connected and don't plan on connecting anymore either.
		 */
		Disconnected,
	};

	static inline std::atomic<State> state = State::Uninitialized;

	/**
	 * API information about the current map.
	 */
	static inline struct
	{
		std::mutex mutex;
		std::optional<KZ::api::messages::MapInfo> info;
	} currentMap;

	/**
	 * Functions that must be executed on the main thread.
	 *
	 * These are used for lifecycle management and state transitions that need to happen on the main thread
	 * (e.g. because we need to get the curren map name).
	 */
	static inline struct
	{
		std::mutex mutex;
		std::vector<std::function<void()>> queue;
	} callbacks;

	/**
	 * All the WebSocket-related state.
	 */
	static inline struct WS
	{
		/**
		 * A partially parsed WebSocket message.
		 */
		struct ReceivedMessage
		{
			/**
			 * The message ID.
			 *
			 * This is either the same ID as a message we sent earlier, indicating a response, or 0.
			 */
			u32 id;
			enum class MessageType
			{
				/**
				 * An earlier message we sent triggered an error.
				 */
				Error,

				/**
				 * Responses to messages sent by us.
				 */
				Response,

				/**
				 * An unknown message type.
				 *
				 * If we receive this, it probably means the current plugin version is outdated and does not yet support the new
				 * message type.
				 */
				Unknown,
			} type;

			/**
			 * The rest of the payload, which will be parsed according to `MessageType` later.
			 */
			Json payload;
		};

		// INVARIANT: should be `nullptr` when `state == Uninitialized`, and a valid pointer otherwise
		static inline std::unique_ptr<ix::WebSocket> socket = nullptr;

		/**
		 * WebSocket messages we have received, but not yet processed.
		 *
		 * These are processed on the main thread, and potentially passed into callbacks stored in
		 * `messageCallbacks.queue`.
		 */
		static inline struct
		{
			std::mutex mutex;
			std::vector<ReceivedMessage> queue;
		} receivedMessages;

		/**
		 * Callbacks for sent messages we expect to get a reply to.
		 *
		 * The key is the message ID we used to send the original message, and the API will respond with a message that
		 * has the same ID. The callback itself is given the message data extracted when we received the message.
		 */
		static inline struct
		{
			std::mutex mutex;
			std::unordered_map<u32, std::function<void(ReceivedMessage &&)>> callbacks;
		} messageCallbacks;

		/**
		 * A handle to the thread that continuously sends 'ping' messages to the API, so we don't get disconnected for
		 * inactivity.
		 */
		static inline struct
		{
			// protects `handle`
			std::mutex mutex;
			std::thread handle;

			// used for waiting for heartbeat timeout and shutdown at the same time
			struct
			{
				std::mutex mutex;
				std::condition_variable cv;
			} shutdownNotification;
		} heartbeatThread;

		/**
		 * Callback for `IXWebSocket`; called on every received message.
		 */
		static void OnMessage(const ix::WebSocketMessagePtr &message);

		/**
		 * Helper function called by `OnMessage()` if we get an `Open` message.
		 */
		static void OnOpenMessage();

		/**
		 * Helper function called by `WS_OnMessage()` if we get a `Close` message.
		 */
		static void OnCloseMessage(const ix::WebSocketCloseInfo &closeInfo);

		/**
		 * Helper function called by `WS_OnMessage()` if we get an `Error` message.
		 */
		static void OnErrorMessage(const ix::WebSocketErrorInfo &errorInfo);

		/**
		 * Initiates the WebSocket handshake with the API.
		 *
		 * This sends the `hello` message.
		 */
		static void InitiateHandshake();

		/**
		 * Completes the WebSocket handshake with the API.
		 *
		 * This is called once we have received the `hello_ack` response to our `hello` message we sent earlier in
		 * `WS_InitiateHandshake()`.
		 */
		static void CompleteHandshake(KZ::api::messages::handshake::HelloAck &&ack);

		/**
		 * The function given to `heartbeatThread` to execute.
		 *
		 * It continuously sends ping messages to ensure we don't get disconnected for inactivity.
		 */
		// No, we can't pass `std::chrono::seconds`, some BS about rvalues.
		// And no, we can't use IXWebSocket's `setPingInterval()` function, because calling that after the connection has already been established
		// seems to do nothing?
		static void Heartbeat(u64 intervalInSeconds);

		/**
		 * Sends the given payload as a WebSocket message.
		 */
		template<typename Payload>
		static bool SendMessage(const Payload &payload)
		{
			u32 messageID;
			const char *messageType;
			Json messagePayload;
			return SendMessageImpl(messageID, messageType, messagePayload, payload);
		}

		/**
		 * Sends the given payload as a WebSocket message and queues the given `callback` to be executed when a response is received.
		 */
		template<typename Payload, typename Callback>
		static bool SendMessage(const Payload &payload, Callback &&callback)
		{
			u32 messageID;
			const char *messageType;
			Json messagePayload;
			if (!SendMessageImpl(messageID, messageType, messagePayload, payload))
			{
				return false;
			}

			auto onReplyCallback = [callback = std::move(callback)](ReceivedMessage &&receivedMessage)
			{
				switch (receivedMessage.type)
				{
					case ReceivedMessage::MessageType::Error:
					{
						KZ::api::messages::Error decodedPayload;
						decodedPayload.FromJson(receivedMessage.payload);

						META_CONPRINTF("[KZ::Global] Received error response to WebSocket message (id=%i): %s\n", receivedMessage.id,
									   decodedPayload.message.c_str());
					}
					break;

					case ReceivedMessage::MessageType::Response:
					{
						std::remove_reference_t<typename decltype(std::function(callback))::argument_type> decodedPayload;
						receivedMessage.payload.Decode(decodedPayload);
						return callback(std::move(decodedPayload));
					}
					break;

					case ReceivedMessage::MessageType::Unknown:
					{
						META_CONPRINTF("[KZ::Global] Received unknown payload as WebSocket response. (id=%i)\n");
					}
					break;
				}
			};

			{
				std::lock_guard _guard(messageCallbacks.mutex);
				messageCallbacks.callbacks.emplace(messageID, std::move(onReplyCallback));
			}

			return true;
		}

	private:
		/**
		 * Implementation detail of `SendMessage()`.
		 *
		 * It initializes the first 3 parameters and encodes `Payload`.
		 */
		template<typename Payload>
		static bool SendMessageImpl(u32 &messageID, const char *&messageType, Json &messagePayload, const Payload &payload)
		{
			static_persist std::atomic<u32> nextMessageID = 1;

			messageID = nextMessageID.fetch_add(1, std::memory_order_relaxed);
			messageType = Payload::Name();

			if (!messagePayload.Set("id", messageID))
			{
				return false;
			}

			if (!messagePayload.Set("type", messageType))
			{
				return false;
			}

			if (!messagePayload.Set("data", payload))
			{
				return false;
			}

			std::string encodedPayload = messagePayload.ToString();
			socket->send(encodedPayload);

			// clang-format off
			META_CONPRINTF("[KZ::Global] Sent WebSocket message. (id=%i, type=%s)\n"
					"------------------------------------\n"
					"%s\n"
					"------------------------------------\n",
					messageID, messageType, encodedPayload.c_str());
			// clang-format on

			return true;
		}
	} ws;

	/**
	 * Information about the current player we received from the API when the player connected.
	 */
	struct
	{
		bool isBanned;
	} playerInfo;

public:
	static void Init();
	static void Cleanup();

	inline static bool IsAvailable()
	{
		return state.load() == State::HandshakeCompleted;
	}

	template<typename F>
	inline static auto WithCurrentMap(F &&f)
	{
		std::lock_guard _guard(currentMap.mutex);
		const std::optional<KZ::api::messages::MapInfo> &mapInfo = currentMap.info;
		return f(mapInfo);
	}

	struct GlobalModeChecksums
	{
		std::string vanilla;
		std::string classic;
	};

	template<typename F>
	inline static auto WithGlobalModes(F &&f)
	{
		std::lock_guard _guard(globalModes.mutex);
		const GlobalModeChecksums &checksums = globalModes.checksums;
		return f(checksums);
	}

	static void OnActivateServer();
	static void OnServerGamePostSimulate();

	static void EnforceConVars();
	static void RestoreConVars();

	void OnPlayerAuthorized();
	void OnClientDisconnect();

	/**
	 * Whether the player is globally banned.
	 */
	bool IsBanned() const
	{
		return this->playerInfo.isBanned;
	}

private:
	/**
	 * API information about modes.
	 */
	static inline struct
	{
		std::mutex mutex;
		GlobalModeChecksums checksums;
	} globalModes;
};
