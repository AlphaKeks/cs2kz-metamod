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
#include "heartbeat.h"
#include "map.h"
#include "events/map_change.h"
#include "kz/option/kz_option.h"
#include "utils/json.h"
#include "utils/http.h"

#include <ixwebsocket/IXNetSystem.h>

std::string KZGlobalService::apiUrl = "https://api.cs2kz.org";
std::string KZGlobalService::apiKey = "";
ix::WebSocket *KZGlobalService::apiSocket = nullptr;
f64 KZGlobalService::heartbeatInterval = 0.0;
u64 KZGlobalService::nextMessageId = 1;
std::vector<KZGlobalService::Callback> KZGlobalService::callbacks {};
static_global bool initialized = false;
static_global bool handshakeDone = false;

static_function void HeartbeatThread();

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
		KZGlobalService::Init();
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
			}
			else
			{
				META_CONPRINTF("[KZ::Global] %s is not global.\n", currentMapName.Get());
			}
		};

		KZGlobalService::FetchMap(currentMapName, callback);
	}
}

void KZGlobalService::FetchMap(u16 mapId, std::function<void(std::optional<KZ::API::Map>)> callback)
{
	META_CONPRINTF("[KZ::Global] TODO: FetchMap(u16)\n");

	callback(std::nullopt);
}

void KZGlobalService::FetchMap(const char *mapName, std::function<void(std::optional<KZ::API::Map>)> callback)
{
	KZ::API::Events::MapChange mapChange {std::string(mapName)};
	KZ::API::Message<KZ::API::Events::MapChange> message("map-change", mapChange);

	auto ws_callback = [callback](json raw_response) mutable
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

template<typename T>
void KZGlobalService::SendMessage(KZ::API::Message<T> message, std::function<void(json)> callback)
{
	message.id = KZGlobalService::nextMessageId++;

	json payload = message;
	KZGlobalService::callbacks.push_back({message.id, callback});
	KZGlobalService::apiSocket->send(payload.dump());
	META_CONPRINTF("[KZ::Global] sent `%s`\n", payload.dump().c_str());
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

				META_CONPRINTF("[KZ::Global] Received 'HelloAck' (heartbeat_interval=%fs)\n", ack.heartbeat_interval);

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

				std::thread(HeartbeatThread).detach();

				handshakeDone = true;
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

			for (auto cb = KZGlobalService::callbacks.begin(); cb != KZGlobalService::callbacks.end(); cb++)
			{
				if (cb->messageId == messageId)
				{
					META_CONPRINTF("[KZ::Global] Executing callback for message #%d.\n", messageId);
					cb->callback(payload);
					META_CONPRINTF("[KZ::Global] Executed callback for message #%d.\n", messageId);
					KZGlobalService::callbacks.erase(cb);
					break;
				}
			}

			break;
		}

		case ix::WebSocketMessageType::Open:
		{
			META_CONPRINTF("[KZ::Global] WebSocket connection established.\n");
			PerformHandshake(message);
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
		u64 steam_id = player->GetSteamId64();

		if (steam_id != 0)
		{
			hello.players.push_back({player->GetName(), steam_id});
		}
	}

	json helloJson = hello;

	KZGlobalService::apiSocket->send(helloJson.dump());

	META_CONPRINTF("[KZ::Global] Sent 'Hello' message.\n");
}

f64 KZGlobalService::Heartbeat()
{
	META_CONPRINTF("[KZ::Global] HeartBeat() interval=%fs\n", KZGlobalService::heartbeatInterval);

	if (!KZGlobalService::Connected())
	{
		META_CONPRINTF("[KZ::Global] Cannot heartbeat while disconnected.\n");

		// TODO: adjust once we have proper retries
		return KZGlobalService::heartbeatInterval * 0.5;
	}

	KZ::API::Heartbeat heartbeat;

	for (Player *player : g_pPlayerManager->players)
	{
		u64 steam_id = player->GetSteamId64();

		if (steam_id != 0)
		{
			heartbeat.players.push_back({player->GetName(), steam_id});
		}
	}

	KZ::API::Message<KZ::API::Heartbeat> message("heartbeat", heartbeat);
	json payload = message;

	KZGlobalService::apiSocket->send(payload.dump());

	META_CONPRINTF("[KZ::Global] Sent heartbeat.\n");

	return KZGlobalService::heartbeatInterval;
}

void KZGlobalService::HeartbeatThread()
{
	f64 heartbeatInterval = Heartbeat();

	while (heartbeatInterval > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds((u64)(heartbeatInterval * 1000)));
		heartbeatInterval = Heartbeat();
	}
}
