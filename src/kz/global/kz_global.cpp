// required for ws library
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#endif

#include <string_view>

#include <ixwebsocket/IXNetSystem.h>

#include "common.h"
#include "cs2kz.h"
#include "kz/kz.h"
#include "kz/option/kz_option.h"
#include "kz_global.h"
#include "kz/global/handshake.h"

#include <vendor/ClientCvarValue/public/iclientcvarvalue.h>

extern IClientCvarValue *g_pClientCvarValue;

static_function OnMessage(const ix::WebSocketMessagePtr &message)
{
	switch (message->type)
	{
		case ix::WebSocketMessageType::Open:
		{
			META_CONPRINTF("[KZ::Global] Connection established!\n");
			*KZGlobalService::state = KZGlobalService::State::Connected;
			KZGlobalService::AddMainThreadCallback(InitiateHandshake);
		}
		break;

		case ix::WebSocketMessageType::Close:
		{
			META_CONPRINTF("[KZ::Global] Connection closed (code %i): %s\n", message->closeInfo.code, message->closeInfo.reason.c_str());

			switch (message->closeInfo.code)
			{
				case 1000 /* NORMAL */:
				case 1001 /* GOING AWAY */:
				case 1006 /* ABNORMAL */: /* fall-through */
				{
					KZGlobalService::socket->enableAutomaticReconnection();
					*KZGlobalService::state = KZGlobalService::State::DisconnectedButWorthRetrying;
				}
				break;

				default:
				{
					KZGlobalService::socket->disableAutomaticReconnection();
					*KZGlobalService::state = KZGlobalService::State::Disconnected;
				}
			}
		}
		break;

		case ix::WebSocketMessageType::Error:
		{
			META_CONPRINTF("[KZ::Global] WebSocket error (code %i): %s\n", message->errorInfo.http_status, message->errorInfo.reason.c_str());

			switch (message->errorInfo.http_status)
			{
				case 401:
				{
					KZGlobalService::socket->disableAutomaticReconnection();
					*KZGlobalService::state = KZGlobalService::State::Disconnected;
				}
				break;

				case 429:
				{
					META_CONPRINTF("[KZ::Global] API rate limit reached; increasing down reconnection delay...\n");
					KZGlobalService::socket->enableAutomaticReconnection();
					KZGlobalService::socket->setMinWaitBetweenReconnectionRetries(10'000 /* ms */);
					KZGlobalService::socket->setMaxWaitBetweenReconnectionRetries(30'000 /* ms */);
					*KZGlobalService::state = KZGlobalService::State::DisconnectedButWorthRetrying;
				}
				break;

				case 500: /* fall-through */
				case 502:
				{
					META_CONPRINTF("[KZ::Global] API encountered an internal error\n");
					KZGlobalService::socket->disableAutomaticReconnection();
					*KZGlobalService::state = KZGlobalService::State::Disconnected;
				}
				break;

				default:
				{
					KZGlobalService::socket->disableAutomaticReconnection();
					*KZGlobalService::state = KZGlobalService::State::Disconnected;
				}
			}
		}
		break;

		case ix::WebSocketMessageType::Message:
		{
			META_CONPRINTF("[KZ::Global] Received WebSocket message:\n-----\n%s\n------\n", message->str.c_str());

			switch (*KZGlobalService::state)
			{
				case KZGlobalService::State::HandshakeInitiated:
				{
					// clang-format off
					KZGlobalService::AddMainThreadCallback([payload = Json(message->str)] ()
					{
						KZ::API::handshake::HelloAck helloAck;

						if (!helloAck.FromJson(payload))
						{
							META_CONPRINTF("[KZ::Global] Failed to decode 'HelloAck'\n");
							return;
						}

						CompleteHandshake(helloAck);
					});
					// clang-format on
				}
				break;

				case KZGlobalService::State::HandshakeCompleted:
				{
					// clang-format off
					KZGlobalService::AddMainThreadCallback([payload = Json(message->str)]()
					{
						if (!payload.IsValid())
						{
							META_CONPRINTF("[KZ::Global] WebSocket message is not valid JSON.\n");
							return;
						}

						u32 messageID = 0;

						if (!payload.Get("id", messageID))
						{
							META_CONPRINTF("[KZ::Global] Ignoring message without valid ID\n");
							return;
						}

						KZGlobalService::ExecuteMessageCallback(messageID, payload);
					});
					// clang-format on
				}
				break;
			}
		}
		break;

		case ix::WebSocketMessageType::Ping:
		{
			META_CONPRINTF("[KZ::Global] Ping!\n");
		}
		break;

		case ix::WebSocketMessageType::Pong:
		{
			META_CONPRINTF("[KZ::Global] Pong!\n");
		}
		break;
	}
}

static_function InitiateHandshake()
{
	bool mapNameOk = false;
	CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&mapNameOk);

	if (!mapNameOk)
	{
		META_CONPRINTF("[KZ::Global] Failed to get current map name. Cannot initiate handshake.\n");
		return;
	}

	std::string_view event("hello");
	KZ::API::handshake::Hello data(g_KZPlugin.GetMD5(), currentMapName.Get());

	for (Player *player : g_pPlayerManager->players)
	{
		if (player && player->IsAuthenticated())
		{
			data.AddPlayer(player->GetSteamId64(), player->GetName());
		}
	}

	KZGlobalService::SendMessage(event, data);
	*KZGlobalService::state = State::HandshakeInitiated;
}

static_function CompleteHandshake(KZ::API::handshake::HelloAck &helloAck)
{
	*KZGlobalService::state = State::HandshakeCompleted;

	// clang-format off
	std::thread([heartbeatInterval = ack.heartbeatInterval]()
	{
		while (*KZGlobalService::state == State::HandshakeCompleted) {
			KZGlobalService::socket->ping("");
			META_CONPRINTF("[KZ::Global] Sent heartbeat. (interval=%is)\n", heartbeatInterval.count());
			std::this_thread::sleep_for(heartbeatInterval);
		}
	}).detach();
	// clang-format on

	{
		std::unique_lock lock(KZGlobalService::currentMap.mutex);
		KZGlobalService::currentMap.data = std::move(helloAck.mapInfo);
	}

	META_CONPRINTF("[KZ::Global] Completed handshake!\n");
}

bool KZGlobalService::IsAvailable()
{
	return *KZGlobalService::state == KZGlobalService::State::HandshakeCompleted;
}

void KZGlobalService::UpdateRecordCache()
{
	u16 currentMapID = 0;

	{
		std::unique_lock lock(KZGlobalService::currentMap.mutex);

		if (!KZGlobalService::currentMap.data.has_value())
		{
			return;
		}

		currentMapID = KZGlobalService::currentMap.data->id;
	}

	std::string_view event("want-world-records-for-cache");
	KZ::API::events::WantWorldRecordsForCache data(currentMapID);
	auto callback = [](const KZ::API::events::WorldRecordsForCache &records)
	{
		for (const KZ::API::Record &record : records.records)
		{
			const KZCourse *course = KZ::course::GetCourseByGlobalCourseID(record.course.id);

			if (course == nullptr)
			{
				META_CONPRINTF("[KZ::Global] Could not find current course?\n");
				continue;
			}

			PluginId modeID = KZ::mode::GetModeInfo(record.mode).id;

			KZTimerService::InsertRecordToCache(record.time, course, modeID, record.nubPoints != 0, true);
		}
	};

	switch (*KZGlobalService::state)
	{
		case KZGlobalService::State::HandshakeCompleted:
			KZGlobalService::SendMessage(event, data, callback);
			break;

		case KZGlobalService::State::Disconnected:
			break;

		default:
			KZGlobalService::AddWhenConnectedCallback([=]() { KZGlobalService::SendMessage(event, data, callback); });
	}
}

void KZGlobalService::Init()
{
	if (*KZGlobalService::state != KZGlobalService::State::Uninitialized)
	{
		return;
	}

	META_CONPRINTF("[KZ::Global] Initializing GlobalService...\n");

	std::string url = KZOptionService::GetOptionStr("apiUrl", "https://api.cs2kz.org");
	std::string_view key = KZOptionService::GetOptionStr("apiKey");

	if (url.empty())
	{
		META_CONPRINTF("[KZ::Global] `apiUrl` is empty. GlobalService will be disabled.\n");
		*KZGlobalService::state = KZGlobalService::State::Disconnected;
		return;
	}

	if (!url.starts_with("http"))
	{
		META_CONPRINTF("[KZ::Global] `apiUrl` is invalid. GlobalService will be disabled.\n");
		*KZGlobalService::state = KZGlobalService::State::Disconnected;
		return;
	}

	if (key.empty())
	{
		META_CONPRINTF("[KZ::Global] `apiKey` is empty. GlobalService will be disabled.\n");
		*KZGlobalService::state = KZGlobalService::State::Disconnected;
		return;
	}

	url.replace(0, 4, "ws");

	if (!url.ends_with("/auth/cs2"))
	{
		if (!url.ends_with("/"))
		{
			url += "/";
		}

		url += "auth/cs2";
	}

	ix::initNetSystem();

	KZGlobalService::socket = new ix::WebSocket();
	KZGlobalService::socket->setUrl(KZGlobalService::config.url);

	// ix::WebSocketHttpHeaders headers;
	// headers["Authorization"] = "Bearer ";
	// headers["Authorization"] += key;
	KZGlobalService::socket->setExtraHeaders({
		{"Authorization", std::string("Bearer ") + key},
	});

	KZGlobalService::socket->setOnMessageCallback(OnMessage);
	KZGlobalService::socket->start();

	KZGlobalService::EnforceConVars();

	*KZGlobalService::state = KZGlobalService::State::Initialized;
}

void KZGlobalService::Cleanup()
{
	if (*KZGlobalService::state == KZGlobalService::State::Uninitialized)
	{
		return;
	}

	META_CONPRINTF("[KZ::Global] Cleaning up GlobalService...\n");

	*KZGlobalService::state = KZGlobalService::State::Uninitialized;

	KZGlobalService::RestoreConVars();

	KZGlobalService::socket->stop();
	delete KZGlobalService::socket.exchange(nullptr);

	ix::uninitNetSystem();
}

void KZGlobalService::OnServerGamePostSimulate()
{
	std::vector<std::function<void()>> callbacks;

	{
		std::unique_lock lock(KZGlobalService::mainThreadCallbacks.mutex, std::defer_lock);

		if (lock.try_lock())
		{
			KZGlobalService::mainThreadCallbacks.queue.swap(callbacks);

			if (*KZGlobalService::state == KZGlobalService::State::HandshakeCompleted)
			{
				callbacks.reserve(callbacks.size() + KZGlobalService::mainThreadCallbacks.whenConnectedQueue.size());
				callbacks.insert(callbacks.end(), KZGlobalService::mainThreadCallbacks.whenConnectedQueue);
				KZGlobalService::mainThreadCallbacks.whenConnectedQueue.clear();
			}
		}
	}

	for (const std::function<void()> &callback : callbacks)
	{
		callback();
	}
}

void KZGlobalService::OnActivateServer()
{
	switch (*KZGlobalService::state)
	{
		case KZGlobalService::State::Uninitialized:
		{
			KZGlobalService::Init();
		}
		break;

		case KZGlobalService::State::Connected:
		{
			bool mapNameOk = false;
			CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&mapNameOk);

			if (!mapNameOk)
			{
				META_CONPRINTF("[KZ::Global] Failed to get current map name. Cannot send `map-change` event.\n");
				return;
			}

			std::string_view event("map-change");
			KZ::API::events::MapChange data(currentMapName.Get());

			// clang-format off
			KZGlobalService::SendMessage(event, data, [currentMapName](KZ::API::events::MapInfo&& mapInfo)
			{
				if (mapInfo.data.has_value())
				{
					META_CONPRINTF("[KZ::Global] %s is approved.\n", mapInfo.data->name);
				}
				else
				{
					META_CONPRINTF("[KZ::Global] %s is not approved.\n", currentMapName.Get());
				}

				{
					std::unique_lock lock(KZGlobalService::currentMap.mutex);
					KZGlobalService::currentMap.data = std::move(mapInfo);
				}
			});
			// clang-format on
		}
		break;
	}
}

void KZGlobalService::OnPlayerAuthorized()
{
	if (*KZGlobalService::state == KZGlobalService::State::Disconnected)
	{
		return;
	}

	// TODO
}

void KZGlobalService::OnClientDisconnect()
{
	if (*KZGlobalService::state == KZGlobalService::State::Disconnected)
	{
		return;
	}

	// TODO
}

void KZGlobalService::ExecuteMessageCallback(u32 messageID, const Json &payload)
{
	std::function<void(const Json &)> callback;

	{
		std::unique_lock lock(KZGlobalService::messageCallbacks.mutex);
		std::unordered_map<u32, std::function<void(u32, const Json &)>> &callbacks = KZGlobalService::messageCallbacks.queue;

		if (auto found = callbacks.extract(messageID); !found.empty())
		{
			callback = std::move(found);
		}
	}

	if (callback)
	{
		META_CONPRINTF("[KZ::Global] Executing callback #%i\n", messageID);
		callback(messageID, payload);
	}
}
