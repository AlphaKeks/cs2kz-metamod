#include <string>

#include "vendor/nlohmann/json.hpp"

#include "../language/kz_language.h"
#include "kz_global.h"
#include "types/players.h"
#include "types/error.h"
#include "utils/http.h"

internal void FetchPlayerImpl(KZPlayer *player, const std::string &url, std::function<void(KZ::API::FullPlayer)> callback)
{
	auto on_response = [player, callback](HTTPRequestHandle request, int status, std::string rawBody)
	{
		const auto json = nlohmann::json::parse(rawBody);

		switch (status)
		{
			case 200:
			{
				const auto playerInfo = KZ::API::FullPlayer::Deserialize(json);
				callback(playerInfo);
				break;
			}

			case 404:
			{
				player->languageService->PrintChat(true, false, "Player not found");
				break;
			}

			default:
			{
				const auto error = KZ::API::Error::Deserialize(json, status);
				error.Report(player);
			}
		}
	};

	g_HTTPManager.Get(url.c_str(), on_response);
}

void KZGlobalService::FetchPlayer(KZPlayer *player, std::function<void(KZ::API::FullPlayer)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	u64 steamID = player->GetSteamId64();
	sprintf(&url.back(), "%llu", steamID);
	FetchPlayerImpl(player, url, callback);
}

void KZGlobalService::FetchPlayer(KZPlayer *player, u64 steamID, std::function<void(KZ::API::FullPlayer)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	sprintf(&url.back(), "%llu", steamID);
	FetchPlayerImpl(player, url, callback);
}

void KZGlobalService::FetchPlayer(KZPlayer *player, const char *name, std::function<void(KZ::API::FullPlayer)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	sprintf(&url.back(), "%s", name);
	FetchPlayerImpl(player, url, callback);
}

void KZGlobalService::FetchPreferences(KZPlayer *player, std::function<void(nlohmann::json)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	u64 steamID = player->GetSteamId64();
	sprintf(&url.back(), "%llu/preferences", steamID);

	auto on_response = [player, callback](HTTPRequestHandle request, int status, std::string rawBody)
	{
		switch (status)
		{
			case 200:
				// good
				break;

			default:
				player->languageService->PrintChat(true, false, "API Error", status, rawBody.c_str());
				return;
		}

		const auto json = nlohmann::json::parse(rawBody);

		callback(json);
	};

	g_HTTPManager.Get(url.c_str(), on_response);
}
