// required for ws library
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#endif

#include <thread>
#include <chrono>

#include "kz_global.h"
#include "../kz.h"
#include "hello.h"
#include "message.h"
#include "map.h"
#include "preferences.h"
#include "events/map_change.h"
#include "events/get_map.h"
#include "events/player_count_change.h"
#include "events/player_update.h"
#include "events/get_preferences.h"
#include "kz/option/kz_option.h"
#include "utils/json.h"
#include "utils/http.h"

#include <ixwebsocket/IXNetSystem.h>

std::optional<KZ::API::Map> KZGlobalService::currentMap = std::nullopt;
std::string KZGlobalService::apiUrl = "https://api.cs2kz.org";
std::string KZGlobalService::apiKey = "";
ix::WebSocket *KZGlobalService::apiSocket = nullptr;
f64 KZGlobalService::heartbeatInterval = 0.0;
u64 KZGlobalService::nextMessageId = 1;
std::vector<KZGlobalService::Callback> KZGlobalService::callbacks {};
static_global bool initialized = false;
static_global bool handshakeDone = false;

void KZGlobalService::Init()
{
	if (initialized)
	{
		return;
	}

	META_CONPRINTF("[KZ::Global] Initializing GlobalService...\n");

#ifdef _WIN32
	ix::initNetSystem();
#endif

	KZGlobalService::apiUrl = KZOptionService::GetOptionStr("apiUrl", "https://api.cs2kz.org");
	KZGlobalService::apiKey = KZOptionService::GetOptionStr("apiKey");

	if (KZGlobalService::apiUrl.rfind("http", 0) != 0)
	{
		META_CONPRINTF("[KZ::Global] Invalid API URL protocol.\n");
		return;
	}

	if (KZGlobalService::apiKey[0] == '\0')
	{
		META_CONPRINTF("[KZ::Global] No API key found.\n");
		return;
	}

	std::string wsUrl = std::string(KZGlobalService::apiUrl).replace(0, 4, "ws") + "/servers/ws";
	ix::WebSocketHttpHeaders wsHeaders;
	wsHeaders["Authorization"] = std::string("Bearer ") + KZGlobalService::apiKey;

	KZGlobalService::apiSocket = new ix::WebSocket();
	KZGlobalService::apiSocket->setUrl(wsUrl);
	KZGlobalService::apiSocket->setExtraHeaders(wsHeaders);
	KZGlobalService::apiSocket->setOnMessageCallback(KZGlobalService::OnMessageCallback);

	// TODO: do we want automatic retries?
	KZGlobalService::apiSocket->disableAutomaticReconnection();

	META_CONPRINTF("[KZ::Global] Establishing WebSocket connection... url=%s\n", wsUrl.c_str());
	KZGlobalService::apiSocket->start();
	initialized = true;
}

void KZGlobalService::OnActivateServer()
{
	if (!initialized)
	{
		Init();
		return;
	}

	if (!KZGlobalService::Connected())
	{
		return;
	}

	bool getMapNameSuccess = false;
	CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&getMapNameSuccess);

	if (getMapNameSuccess)
	{
		auto callback = [currentMapName](std::optional<KZ::API::Map> map)
		{
			if (map.has_value())
			{
				META_CONPRINTF("[KZ::Global] Fetched map %s with ID %d.\n", map->name.c_str(), map->id);
				KZGlobalService::currentMap = map.value();
			}
			else
			{
				META_CONPRINTF("[KZ::Global] %s is not global.\n", currentMapName.Get());
			}
		};

		KZ::API::Events::MapChange mapChange {std::string(currentMapName)};
		KZ::API::Message<KZ::API::Events::MapChange> message("map-change", mapChange);

		auto ws_callback = [callback](json raw_response)
		{
			KZ::API::Message<std::optional<KZ::API::Map>> response("map-info", std::nullopt);

			if (!raw_response["payload"].is_null())
			{
				response.payload = raw_response["payload"];
			}

			callback(response.payload);
		};

		KZGlobalService::SendMessage(message, ws_callback);
	}
}

void KZGlobalService::OnPlayerJoinTeam(i32 team)
{
	switch (team)
	{
		case CS_TEAM_T: // fallthrough
		case CS_TEAM_CT:
		{
			this->session.SwitchState(KZ::API::GameSession::PlayerState::ACTIVE);
			break;
		}
		case CS_TEAM_SPECTATOR:
		{
			this->session.SwitchState(KZ::API::GameSession::PlayerState::SPECTATING);
			break;
		}
	}
}

void KZGlobalService::PlayerCountChange(KZPlayer *currentPlayer)
{
	KZ::API::Events::PlayerCountChange event;
	event.max_players = g_pKZUtils->GetServerGlobals()->maxClients;

	for (Player *player : g_pKZPlayerManager->players)
	{
		KZPlayer *kzPlayer = static_cast<KZPlayer *>(player);

		if (!kzPlayer->IsConnected())
		{
			continue;
		}

		if (currentPlayer == kzPlayer)
		{
			continue;
		}

		event.total_players++;

		if (kzPlayer->IsAuthenticated())
		{
			event.authenticated_players.push_back({kzPlayer->GetName(), kzPlayer->GetSteamId64(), kzPlayer->GetIpAddress()});
		}
	}

	KZ::API::Message<KZ::API::Events::PlayerCountChange> message("player-count-change", event);

	KZGlobalService::SendMessage(message);
}

void KZGlobalService::PlayerDisconnect()
{
	if (!this->player->IsAuthenticated())
	{
		return;
	}

	// Flush timestamps
	this->session.SwitchState(KZ::API::GameSession::PlayerState::ACTIVE);

	u64 steamID = this->player->GetSteamId64();

	CUtlString getPrefsError;
	CUtlString getPrefsResult;
	this->player->optionService->GetPreferencesAsJSON(&getPrefsError, &getPrefsResult);

	if (!getPrefsError.IsEmpty())
	{
		META_CONPRINTF("[KZ::Global] Failed to get preferences: %s\n", getPrefsError.Get());
		META_CONPRINTF("[KZ::Global] Not sending `PlayerDisconnect` event.\n");
		return;
	}

	KZ::API::PlayerInfo playerInfo = {this->player->GetName(), steamID, this->player->GetIpAddress()};
	json preferences = json::parse(getPrefsResult.Get());

	KZ::API::Events::PlayerUpdate event {playerInfo, preferences, this->session};
	KZ::API::Message<KZ::API::Events::PlayerUpdate> message("player-update", event);

	KZGlobalService::SendMessage(message);
	KZGlobalService::PlayerCountChange(this->player);
}

void KZGlobalService::FetchMap(const char *mapName, std::function<void(std::optional<KZ::API::Map>)> callback)
{
	KZ::API::Events::GetMap<std::string> getMap(mapName);
	KZ::API::Message<KZ::API::Events::GetMap<std::string>> message("get-map", getMap);

	auto ws_callback = [callback](json raw_response)
	{
		KZ::API::Message<std::optional<KZ::API::Map>> response("map-info", std::nullopt);

		if (!raw_response["payload"].is_null())
		{
			response.payload = raw_response["payload"];
		}

		callback(response.payload);
	};

	KZGlobalService::SendMessage(message, ws_callback);
}

void KZGlobalService::InitializePreferences()
{
	KZ::API::Events::GetPreferences event {this->player->GetSteamId64()};
	KZ::API::Message<KZ::API::Events::GetPreferences> message("get-preferences", event);

	auto ws_callback = [userID = this->player->GetClient()->GetUserID()](json raw_response)
	{
		KZPlayer *player = g_pKZPlayerManager->ToPlayer(userID);

		if (!player)
		{
			return;
		}

		KZ::API::Message<KZ::API::Preferences> response("preferences", {});
		response.payload = raw_response["payload"];

		player->optionService->InitializeGlobalPrefs(response.payload.preferences.dump());
	};

	KZGlobalService::SendMessage(message, ws_callback);
}

void KZGlobalService::Cleanup()
{
	if (KZGlobalService::apiSocket)
	{
		delete KZGlobalService::apiSocket;
		KZGlobalService::apiSocket = nullptr;
	}

#ifdef _WIN32
	ix::uninitNetSystem();
#endif
}

f64 KZGlobalService::Heartbeat()
{
	if (!KZGlobalService::Connected())
	{
		META_CONPRINTF("[KZ::Global] Cannot heartbeat while disconnected.\n");

		// TODO: adjust once we have proper retries
		return KZGlobalService::heartbeatInterval;
	}

	KZGlobalService::apiSocket->ping("");

	META_CONPRINTF("[KZ::Global] Sent heartbeat. (interval=%.2fs)\n", KZGlobalService::heartbeatInterval);

	return KZGlobalService::heartbeatInterval;
}

void KZGlobalService::HeartbeatThread()
{
	if (KZGlobalService::apiSocket == nullptr)
	{
		return;
	}

	f64 heartbeatInterval = KZGlobalService::Heartbeat();

	while (heartbeatInterval > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds((u64)(heartbeatInterval * 1000)));
		heartbeatInterval = KZGlobalService::Heartbeat();
	}
}

void KZGlobalService::OnMessageCallback(const ix::WebSocketMessagePtr &message)
{
	switch (message->type)
	{
		case ix::WebSocketMessageType::Message:
		{
			META_CONPRINTF("[KZ::Global] WebSocket message: `%s`\n", message->str.c_str());

			json payload = json::parse(message->str);

			if (!payload.is_object())
			{
				META_CONPRINTF("[KZ::Global] Received text/binary message that wasn't an object\n");
				break;
			}

			if (!handshakeDone)
			{
				KZ::API::HelloAck ack = payload;

				META_CONPRINTF("[KZ::Global] Received 'HelloAck' (heartbeat_interval=%.2fs)\n", ack.heartbeat_interval);

				// We actually send heartbeats slightly more frequently than
				// requested by the API, so that we don't miss any even if
				// there are network issues like a slow connection.
				KZGlobalService::heartbeatInterval = ack.heartbeat_interval * 0.8;

				bool getMapNameSuccess = false;
				CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&getMapNameSuccess);

				if (getMapNameSuccess)
				{
					auto callback = [currentMapName](std::optional<KZ::API::Map> map)
					{
						if (map.has_value())
						{
							META_CONPRINTF("[KZ::Global] Fetched map %s with ID %d.\n", map->name.c_str(), map->id);
						}
						else
						{
							META_CONPRINTF("[KZ::Global] %s is not global.\n", currentMapName.Get());
						}
					};

					KZGlobalService::FetchMap(currentMapName, callback);
				}
				else
				{
					META_CONPRINTF("[KZ::Global] Could not get current map name.\n");
				}

				std::thread(KZGlobalService::HeartbeatThread).detach();

				handshakeDone = true;

				KZGlobalService::PlayerCountChange();

				break;
			}

			if (!payload.contains("id"))
			{
				META_CONPRINTF("[KZ::Global] Received JSON message without an ID: `%s`\n", payload.dump().c_str());
				break;
			}

			if (!payload["id"].is_number_unsigned())
			{
				META_CONPRINTF("[KZ::Global] Received JSON message with weird ID type: `%s`\n", payload.dump().c_str());
				break;
			}

			u64 messageId = payload["id"];

			for (auto cb = callbacks.begin(); cb != callbacks.end(); cb++)
			{
				if (cb->messageId == messageId)
				{
					META_CONPRINTF("[KZ::Global] Executing callback for message #%d.\n", messageId);
					cb->callback(payload);
					META_CONPRINTF("[KZ::Global] Executed callback for message #%d.\n", messageId);
					callbacks.erase(cb);
					break;
				}
			}

			break;
		}

		case ix::WebSocketMessageType::Open:
		{
			META_CONPRINTF("[KZ::Global] WebSocket connection established.\n");
			KZGlobalService::PerformHandshake(message);
			break;
		}

		case ix::WebSocketMessageType::Close:
		{
			META_CONPRINTF("[KZ::Global] WebSocket connection closed.\n");
			break;
		}

		case ix::WebSocketMessageType::Error:
		{
			META_CONPRINTF("[KZ::Global] WebSocket error: `%s`\n", message->errorInfo.reason.c_str());
			break;
		}

		case ix::WebSocketMessageType::Ping:
		{
			META_CONPRINTF("[KZ::Global] Received ping.\n");
			break;
		}

		case ix::WebSocketMessageType::Pong:
		{
			META_CONPRINTF("[KZ::Global] Received Pong.\n");
			break;
		}

		case ix::WebSocketMessageType::Fragment:
		{
			// unused
			break;
		}

		default:
		{
			META_CONPRINTF("[KZ::Global] WebSocket message (type %d)\n", message->type);
		}
	}
}

void KZGlobalService::PerformHandshake(const ix::WebSocketMessagePtr &message)
{
	assert(message->type == ix::WebSocketMessageType::Open);

	META_CONPRINTF("[KZ::Global] Performing handshake...\n");

	bool getMapNameSuccess = false;
	CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&getMapNameSuccess);

	if (!getMapNameSuccess)
	{
		META_CONPRINTF("[KZ::Global] Could not get current map name.\n");
		return;
	}

	KZ::API::Hello hello;
	hello.current_map = currentMapName.Get();

	for (Player *player : g_pPlayerManager->players)
	{
		if (!player->IsAuthenticated())
		{
			continue;
		}

		hello.players.push_back({player->GetName(), player->GetSteamId64()});
	}

	json helloJson = hello;

	KZGlobalService::apiSocket->send(helloJson.dump());

	META_CONPRINTF("[KZ::Global] Sent 'Hello' message.\n");
}

template<typename T>
void KZGlobalService::SendMessage(KZ::API::Message<T> message)
{
	if (!KZGlobalService::Connected())
	{
		return;
	}

	message.id = nextMessageId++;

	json payload = message;
	KZGlobalService::apiSocket->send(payload.dump());

	META_CONPRINTF("[KZ::Global] sent `%s`\n", payload.dump().c_str());
}

template<typename T>
void KZGlobalService::SendMessage(KZ::API::Message<T> message, std::function<void(json)> callback)
{
	if (!KZGlobalService::Connected())
	{
		return;
	}

	message.id = nextMessageId++;

	json payload = message;
	KZGlobalService::callbacks.push_back({message.id, callback});
	KZGlobalService::apiSocket->send(payload.dump());

	META_CONPRINTF("[KZ::Global] sent `%s`\n", payload.dump().c_str());
}
