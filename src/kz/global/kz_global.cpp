#include "kz_global.h"
#include "../kz.h"
#include "hello.h"
#include "message.h"
#include "heartbeat.h"
#include "kz/option/kz_option.h"
#include "utils/json.h"
#include "utils/http.h"
#include "utils/ctimer.h"

std::string KZGlobalService::apiUrl = "https://api.cs2kz.org";
std::string KZGlobalService::apiKey = "";
ix::WebSocket *KZGlobalService::apiSocket = nullptr;
f64 KZGlobalService::heartbeatInterval = 0.0;
u64 KZGlobalService::currentMessageId = 1;
static bool initialized = false;

void KZGlobalService::Init()
{
	if (initialized)
	{
		return;
	}

	META_CONPRINTF("[KZ::Global] Initializing GlobalService...\n");

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
	KZGlobalService::apiSocket->disableAutomaticReconnection();

	META_CONPRINTF("[KZ::Global] Establishing WebSocket connection... url=%s\n", wsUrl.c_str());
	KZGlobalService::apiSocket->start();
	initialized = true;
}

void KZGlobalService::OnMessageCallback(const ix::WebSocketMessagePtr &message)
{
	switch (message->type)
	{
		case ix::WebSocketMessageType::Open:
		{
			META_CONPRINTF("[KZ::Global] WebSocket connection established.\n");
			PerformHandshake(message);
			break;
		}

		case ix::WebSocketMessageType::Close:
		{
			META_CONPRINTF("[KZ::Global] WebSocket connection closed.\n");
			KZGlobalService::heartbeatInterval = 0.0;
			break;
		}

		case ix::WebSocketMessageType::Error:
		{
			META_CONPRINTF("[KZ::Global] WebSocket error: `%s`\n", message->errorInfo.reason.c_str());
			break;
		}

		case ix::WebSocketMessageType::Message:
		{
			META_CONPRINTF("[KZ::Global] WebSocket message: `%s`\n", message->str.c_str());

			if (!KZGlobalService::Connected())
			{
				KZ::API::HelloAck ack = json::parse(message->str);

				META_CONPRINTF("[KZ::Global] Received 'HelloAck' (heartbeat_interval=%f)\n", ack.heartbeat_interval);
				KZGlobalService::heartbeatInterval = ack.heartbeat_interval * 0.8;

				StartTimer(KZGlobalService::Heartbeat, true, true);
			}

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
	if (!KZGlobalService::Connected())
	{
		return 0.0;
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

	KZ::API::Message<KZ::API::Heartbeat> message {KZGlobalService::currentMessageId++, "heartbeat", heartbeat};
	json payload = message;

	KZGlobalService::apiSocket->send(payload.dump());

	META_CONPRINTF("[KZ::Global] Sent heartbeat.\n");

	return KZGlobalService::heartbeatInterval;
}
