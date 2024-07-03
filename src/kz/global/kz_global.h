#pragma once

#include <string>

#include "../kz.h"
#include "common.h"
#include "types/players.h"
#include "types/maps.h"
#include "utils/http.h"

class KZGlobalService : public KZBaseService
{
	using KZBaseService::KZBaseService;

	static_global f64 Heartbeat();
	static_global void OnHeartbeat(HTTPRequestHandle request, int status, std::string body);

public:
	static_global std::string apiUrl;

	static_global void Init();
	static_global void RegisterCommands();

	static_global void FetchPlayer(KZPlayer *player, std::function<void(KZ::API::FullPlayer)> callback);
	static_global void FetchPlayer(KZPlayer *player, u64 steamID, std::function<void(KZ::API::FullPlayer)> callback);
	static_global void FetchPlayer(KZPlayer *player, const char *name, std::function<void(KZ::API::FullPlayer)> callback);
	static_global void FetchPreferences(KZPlayer *player, std::function<void(nlohmann::json)> callback);
	static_global void FetchMap(KZPlayer *player, std::function<void(KZ::API::Map)> callback);
};
