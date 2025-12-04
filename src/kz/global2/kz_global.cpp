// required for ws library
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#endif

#include <string_view>

#include <ixwebsocket/IXNetSystem.h>

#include "kz/kz.h"
#include "kz/option/kz_option.h"
#include "utils/json.h"

#include "kz_global.h"
#include "messages.h"

static_function bool getApiUrl(std::string &url)
{
	url = KZOptionService::GetOptionStr("apiUrl", KZOptionService::GetOptionStr("apiUrl", "https://api.cs2kz.org"));

	if (url.empty())
	{
		META_CONPRINTF("[KZ::Global] `apiUrl` is empty. GlobalService will be disabled.\n");
		return false;
	}

	if (url.size() < 4 || url.substr(0, 4) != "http")
	{
		META_CONPRINTF("[KZ::Global] `apiUrl` is invalid. GlobalService will be disabled.\n");
		return false;
	}

	url.replace(0, 4, "ws");

	if (url.substr(url.size() - 1) != "/")
	{
		url += "/";
	}

	url += "ws";

	return true;
}

static_function bool getApiKey(std::string &key)
{
	key = KZOptionService::GetOptionStr("apiKey");

	if (key.empty())
	{
		META_CONPRINTF("[KZ::Global] `apiKey` is empty. GlobalService will be disabled.\n");
		return false;
	}

	return true;
}

void KZGlobalService2::Init()
{
	if (KZGlobalService2::state.load() != KZGlobalService2::State::Uninitialized)
	{
		return;
	}

	META_CONPRINTF("[KZ::Global] Initializing GlobalService...\n");

	std::string apiUrl;
	if (!getApiUrl(apiUrl))
	{
		KZGlobalService2::state.store(KZGlobalService2::State::Disconnected);
		return;
	}
	META_CONPRINTF("[KZ::Global] API URL is `%s`.\n", apiUrl.c_str());

	std::string apiKey;
	if (!getApiKey(apiKey))
	{
		KZGlobalService2::state.store(KZGlobalService2::State::Disconnected);
		return;
	}

	KZGlobalService2::EnforceConVars();

	ix::initNetSystem();

	KZGlobalService2::WS::socket = std::make_unique<ix::WebSocket>();
	KZGlobalService2::WS::socket->setUrl(apiUrl);
	KZGlobalService2::WS::socket->setExtraHeaders({{"Authorization", std::string("Access-Key ") + apiKey}});
	KZGlobalService2::WS::socket->setOnMessageCallback(KZGlobalService2::WS::OnMessage);

	KZGlobalService2::state.store(KZGlobalService2::State::Configured);
}

void KZGlobalService2::Cleanup()
{
	if (KZGlobalService2::WS::socket)
	{
		KZGlobalService2::WS::socket->stop();
		KZGlobalService2::state.store(KZGlobalService2::State::Disconnected);
		KZGlobalService2::WS::socket.reset(nullptr);

		ix::uninitNetSystem();
	}

	{
		std::lock_guard _guard(KZGlobalService2::ws.heartbeatThread.mutex);
		if (KZGlobalService2::ws.heartbeatThread.handle.joinable())
		{
			KZGlobalService2::ws.heartbeatThread.shutdownNotification.cv.notify_all();
			KZGlobalService2::ws.heartbeatThread.handle.join();
		}
	}

	KZGlobalService2::state.store(KZGlobalService2::State::Uninitialized);
	KZGlobalService2::RestoreConVars();
}

void KZGlobalService2::OnActivateServer()
{
	if (KZGlobalService2::state.load() == KZGlobalService2::State::Uninitialized)
	{
		KZGlobalService2::Init();
	}

	switch (KZGlobalService2::state.load())
	{
		case KZGlobalService2::State::Configured:
		{
			META_CONPRINTF("[KZ::Global] Starting WebSocket...\n");
			KZGlobalService2::WS::socket->start();
			KZGlobalService2::state.store(KZGlobalService2::State::Connecting);
		}
		break;

		case KZGlobalService2::State::HandshakeCompleted:
		{
			KZ::api::messages::MapChange message;

			bool mapNameOk = false;
			CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&mapNameOk);

			if (!mapNameOk)
			{
				META_CONPRINTF("[KZ::Global] Failed to get current map name. Cannot send `map_change` event.\n");
				break;
			}

			message.name = currentMapName.Get();

			auto onMessageInfoCallback = [](std::optional<KZ::api::messages::MapInfo> &&mapInfo)
			{
				{
					std::lock_guard _guard(currentMap.mutex);
					KZGlobalService2::currentMap.info = std::move(mapInfo);
				}
			};

			KZGlobalService2::WS::SendMessage(message, std::move(onMessageInfoCallback));
		}
		break;
	}
}

void KZGlobalService2::OnServerGamePostSimulate()
{
	{
		std::lock_guard _guard(KZGlobalService2::callbacks.mutex);
		if (!KZGlobalService2::callbacks.queue.empty())
		{
			META_CONPRINTF("[KZ::Global] Running callbacks...\n");
			for (const std::function<void()> &callback : KZGlobalService2::callbacks.queue)
			{
				callback();
			}
			KZGlobalService2::callbacks.queue.clear();
		}
	}

	{
		std::lock_guard _messageCallbacksQueueGuard(KZGlobalService2::ws.messageCallbacks.mutex);

		std::lock_guard _messagesQueueGuard(KZGlobalService2::ws.receivedMessages.mutex);
		for (WS::ReceivedMessage &message : KZGlobalService2::ws.receivedMessages.queue)
		{
			auto callbackHandle = KZGlobalService2::ws.messageCallbacks.callbacks.extract(message.id);
			if (callbackHandle)
			{
				std::function<void(WS::ReceivedMessage)> callback = std::move(callbackHandle.mapped());
				META_CONPRINTF("[KZ::Global] Running callback for message %i...\n", message.id);
				callback(std::move(message));
			}
		}
		KZGlobalService2::ws.receivedMessages.queue.clear();
	}
}

void KZGlobalService2::OnPlayerAuthorized()
{
	if (KZGlobalService2::state.load() != KZGlobalService2::State::HandshakeCompleted)
	{
		return;
	}

	if (!this->player->IsConnected())
	{
		return;
	}
	assert(this->player->IsAuthenticated());

	u64 steamID = this->player->GetSteamId64();
	std::string stringifiedSteamID = std::to_string(steamID);

	KZ::api::messages::PlayerJoin message;
	message.id = stringifiedSteamID;
	message.name = this->player->GetName();
	message.ipAddress = this->player->GetIpAddress();

	auto onAckCallback = [=](KZ::api::messages::PlayerJoinAck &&ack)
	{
		META_CONPRINTF("[KZ::Global] Received `player_join_ack` response. (player.id=%llu, player.is_banned=%s)\n", steamID,
					   ack.isBanned ? "true" : "false");

		KZPlayer *player = g_pKZPlayerManager->SteamIdToPlayer(steamID);
		if (player == nullptr)
		{
			return;
		}

		player->globalService2->playerInfo.isBanned = ack.isBanned;
		player->optionService->InitializeGlobalPrefs(ack.preferences.ToString());
	};

	KZGlobalService2::WS::SendMessage(message, std::move(onAckCallback));
}

void KZGlobalService2::OnClientDisconnect()
{
	if (KZGlobalService2::state.load() != KZGlobalService2::State::HandshakeCompleted)
	{
		return;
	}

	if (!this->player->IsConnected() || !this->player->IsAuthenticated())
	{
		return;
	}

	u64 steamID = this->player->GetSteamId64();
	std::string stringifiedSteamID = std::to_string(steamID);

	KZ::api::messages::PlayerLeave message;
	message.id = stringifiedSteamID;
	message.name = this->player->GetName();

	CUtlString getPrefsError;
	CUtlString getPrefsResult;
	this->player->optionService->GetPreferencesAsJSON(&getPrefsError, &getPrefsResult);
	if (getPrefsError.IsEmpty())
	{
		message.preferences = Json(getPrefsResult.Get());
	}

	KZGlobalService2::WS::SendMessage(message);
}

void KZGlobalService2::WS::OnMessage(const ix::WebSocketMessagePtr &message)
{
	switch (message->type)
	{
		case ix::WebSocketMessageType::Open:
			return KZGlobalService2::WS::OnOpenMessage();

		case ix::WebSocketMessageType::Close:
			return KZGlobalService2::WS::OnCloseMessage(message->closeInfo);

		case ix::WebSocketMessageType::Error:
			return KZGlobalService2::WS::OnErrorMessage(message->errorInfo);

		case ix::WebSocketMessageType::Ping:
			return META_CONPRINTF("[KZ::Global] Received ping WebSocket message.\n");

		case ix::WebSocketMessageType::Pong:
			return META_CONPRINTF("[KZ::Global] Received pong WebSocket message.\n");
	}

	META_CONPRINTF("[KZ::Global] Received WebSocket message.\n"
				   "----------------------------------------\n"
				   "%s"
				   "\n----------------------------------------\n",
				   message->str.c_str());

	Json payload(message->str);

	if (!payload.IsValid())
	{
		META_CONPRINTF("[KZ::Global] Incoming WebSocket message is not valid JSON.\n");
		return;
	}

	switch (KZGlobalService2::state.load())
	{
		case KZGlobalService2::State::HandshakeInitiated:
		case KZGlobalService2::State::Reconnecting:
		{
			KZ::api::messages::handshake::HelloAck ack;

			if (!ack.FromJson(payload))
			{
				// maybe?
				// KZGlobalService2::socket->stop();
				KZGlobalService2::state.store(KZGlobalService2::State::Disconnected);
				META_CONPRINTF("[KZ::Global] Failed to decode `hello_ack` message.\n");
				break;
			}

			{
				std::lock_guard _guard(KZGlobalService2::callbacks.mutex);

				// clang-format off
				KZGlobalService2::callbacks.queue.emplace_back([ack = std::move(ack)]() mutable {
					return KZGlobalService2::WS::CompleteHandshake(std::move(ack));
				});
				// clang-format on
			}
		}
		break;

		case KZGlobalService2::State::HandshakeCompleted:
		{
			u32 messageID;
			if (!payload.Get("id", messageID))
			{
				META_CONPRINTF("[KZ::Global] Incoming WebSocket message did not contain a valid `id` field.\n");
				break;
			}

			std::string messageType;
			if (!payload.Get("type", messageType))
			{
				META_CONPRINTF("[KZ::Global] Incoming WebSocket message did not contain a valid `type` field.\n");
				break;
			}

			ReceivedMessage message;
			message.id = messageID;

			if (messageType == "error")
			{
				message.type = ReceivedMessage::MessageType::Error;
			}
			else if ((messageType == "map_info") || (messageType == "player_join_ack"))
			{
				message.type = ReceivedMessage::MessageType::Response;
			}
			else
			{
				META_CONPRINTF("[KZ::Global] Incoming WebSocket message contained an unknown `type` field: `%s`\n", messageType.c_str());
				break;
			}

			if (!payload.Get("data", message.payload))
			{
				META_CONPRINTF("[KZ::Global] Incoming WebSocket message contained an invalid `data` field.\n");
				break;
			}

			{
				std::lock_guard _guard(KZGlobalService2::ws.receivedMessages.mutex);
				KZGlobalService2::ws.receivedMessages.queue.emplace_back(std::move(message));
			}
		}
		break;

		default:
			return;
	}
}

void KZGlobalService2::WS::OnOpenMessage()
{
	KZGlobalService2::state.store(KZGlobalService2::State::Connected);
	META_CONPRINTF("[KZ::Global] WebSocket connection established.\n");
	{
		std::lock_guard _guard(KZGlobalService2::callbacks.mutex);
		KZGlobalService2::callbacks.queue.emplace_back([]() { return KZGlobalService2::WS::InitiateHandshake(); });
	}
}

void KZGlobalService2::WS::OnCloseMessage(const ix::WebSocketCloseInfo &closeInfo)
{
	META_CONPRINTF("[KZ::Global] WebSocket connection closed (%i): %s\n", closeInfo.code, closeInfo.reason.c_str());

	switch (closeInfo.code)
	{
		case 1000 /* NORMAL */:
		case 1001 /* GOING AWAY */:
		case 1006 /* ABNORMAL */:
		{
			KZGlobalService2::WS::socket->enableAutomaticReconnection();
			KZGlobalService2::WS::socket->setMinWaitBetweenReconnectionRetries(10'000 /* ms */);
			KZGlobalService2::state.store(KZGlobalService2::State::Reconnecting);
		}
		break;

		case 1008 /* POLICY VIOLATION */:
		{
			if (closeInfo.reason.find("heartbeat timeout") != -1)
			{
				KZGlobalService2::WS::socket->enableAutomaticReconnection();
				KZGlobalService2::state.store(KZGlobalService2::State::Reconnecting);
			}
			else
			{
				KZGlobalService2::WS::socket->disableAutomaticReconnection();
				KZGlobalService2::state.store(KZGlobalService2::State::Disconnected);
			}
		}
		break;

		default:
		{
			KZGlobalService2::WS::socket->enableAutomaticReconnection();
			KZGlobalService2::WS::socket->setMinWaitBetweenReconnectionRetries(60'000 /* ms */);
			KZGlobalService2::state.store(KZGlobalService2::State::Reconnecting);
		}
	}

	{
		std::lock_guard _guard(KZGlobalService2::ws.heartbeatThread.mutex);
		if (KZGlobalService2::ws.heartbeatThread.handle.joinable())
		{
			KZGlobalService2::ws.heartbeatThread.shutdownNotification.cv.notify_all();
			KZGlobalService2::ws.heartbeatThread.handle.join();
		}
	}
}

void KZGlobalService2::WS::OnErrorMessage(const ix::WebSocketErrorInfo &errorInfo)
{
	META_CONPRINTF("[KZ::Global] WebSocket error (status %i, retries=%i, wait_time=%f): %s\n", errorInfo.http_status, errorInfo.retries,
				   errorInfo.wait_time, errorInfo.reason.c_str());

	switch (errorInfo.http_status)
	{
		case 401:
		case 403:
		{
			KZGlobalService2::WS::socket->disableAutomaticReconnection();
			KZGlobalService2::state.store(KZGlobalService2::State::Disconnected);
		}
		break;

		default:
		{
			KZGlobalService2::WS::socket->enableAutomaticReconnection();
			KZGlobalService2::WS::socket->setMinWaitBetweenReconnectionRetries(60'000 /* ms */);
			KZGlobalService2::state.store(KZGlobalService2::State::Reconnecting);
		}
	}
}

void KZGlobalService2::WS::InitiateHandshake()
{
	KZGlobalService2::State currentState = KZGlobalService2::state.load();

	if (currentState != KZGlobalService2::State::Connected)
	{
		return;
	}

	if (!KZGlobalService2::state.compare_exchange_strong(currentState, KZGlobalService2::State::HandshakeInitiated))
	{
		return;
	}

	META_CONPRINTF("[KZ::Global] Initiating WebSocket handshake...\n");

	KZ::api::messages::handshake::Hello message;
	message.pluginVersion = PLUGIN_FULL_VERSION;
	message.pluginVersionChecksum = g_KZPlugin.GetMD5();

	bool mapNameOk = false;
	CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&mapNameOk);

	if (!mapNameOk)
	{
		META_CONPRINTF("[KZ::Global] Failed to get current map name.\n");
		KZGlobalService2::state.store(KZGlobalService2::State::Disconnected);
		return;
	}

	message.currentMap = currentMapName.Get();

	KZGlobalService2::WS::socket->send(Json(message).ToString());
}

void KZGlobalService2::WS::CompleteHandshake(KZ::api::messages::handshake::HelloAck &&ack)
{
	KZGlobalService2::State currentState = KZGlobalService2::state.load();

	if ((currentState != KZGlobalService2::State::HandshakeInitiated) && (currentState != KZGlobalService2::State::Reconnecting))
	{
		META_CONPRINTF("[KZ::Global] Unexpected state when calling `CompleteHandshake()`.\n");
	}

	META_CONPRINTF("[KZ::Global] Completing WebSocket handshake...\n");
	META_CONPRINTF("[KZ::Global] WebSocket session ID: `%s`\n", ack.sessionID.c_str());

	{
		std::lock_guard _guard(KZGlobalService2::ws.heartbeatThread.mutex);
		if (!KZGlobalService2::ws.heartbeatThread.handle.joinable())
		{
			META_CONPRINTF("[KZ::Global] Spawning WebSocket heartbeat thread...\n");
			KZGlobalService2::ws.heartbeatThread.handle = std::thread(KZGlobalService2::WS::Heartbeat, ack.heartbeatInterval);
		}
	}

	{
		std::lock_guard _guard(KZGlobalService2::currentMap.mutex);
		KZGlobalService2::currentMap.info = std::move(ack.mapInfo);
	}

	{
		std::lock_guard _guard(KZGlobalService2::globalModes.mutex);

#ifdef _WIN32
		KZGlobalService2::globalModes.checksums.vanilla = ack.modeChecksums.vanilla.windows;
		KZGlobalService2::globalModes.checksums.classic = ack.modeChecksums.classic.windows;
#else
		KZGlobalService2::globalModes.checksums.vanilla = ack.modeChecksums.vanilla.linux;
		KZGlobalService2::globalModes.checksums.classic = ack.modeChecksums.classic.linux;
#endif
	}

	// TODO: styles

	if (!KZGlobalService2::state.compare_exchange_strong(currentState, KZGlobalService2::State::HandshakeCompleted))
	{
		META_CONPRINTF("[KZ::Global] State changed unexpectedly during `CompleteHandshake()`.\n");
	}

	for (Player *player : g_pKZPlayerManager->players)
	{
		if (player && player->IsConnected() && player->IsAuthenticated())
		{
			g_pKZPlayerManager->ToKZPlayer(player)->globalService2->OnPlayerAuthorized();
		}
	}
}

void KZGlobalService2::WS::Heartbeat(u64 intervalInSeconds)
{
	assert(intervalInSeconds > 0);

	// `* 800` here is effectively `* 0.8` (in milliseconds), which we do to ensure we send heartbeats frequently
	// enough even under sub-optimal network conditions.
	std::chrono::milliseconds interval(intervalInSeconds * 800);

	std::unique_lock shutdownLock(KZGlobalService2::ws.heartbeatThread.shutdownNotification.mutex);
	KZGlobalService2::State lastKnownState = KZGlobalService2::state.load();

	while (lastKnownState != KZGlobalService2::State::Disconnected)
	{
		// clang-format off
		bool shutdownRequested = KZGlobalService2::ws.heartbeatThread.shutdownNotification.cv.wait_for(shutdownLock, interval, [&]() {
			lastKnownState = KZGlobalService2::state.load();
			return ((lastKnownState == KZGlobalService2::State::Reconnecting)
			     || (lastKnownState == KZGlobalService2::State::Disconnected));
		});
		// clang-format on

		if (shutdownRequested)
		{
			META_CONPRINTF("[KZ::Global] WebSocket heartbeat thread shutting down...\n");
			break;
		}

		KZGlobalService2::WS::socket->ping("");
		META_CONPRINTF("[KZ::Global] Sent heartbeat WebSocket message. (interval=%is)\n",
					   std::chrono::duration_cast<std::chrono::seconds>(interval).count());
	}
}
