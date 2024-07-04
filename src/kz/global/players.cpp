#include <string>

#include "vendor/nlohmann/json.hpp"

#include "../language/kz_language.h"
#include "kz_global.h"
#include "types/players.h"
#include "types/error.h"
#include "utils/http.h"

internal void FetchPlayerImpl(KZPlayer *player, const std::string &url, bool createIfNotExists, std::function<void(KZ::API::FullPlayer)> callback)
{
	g_HTTPManager.Get(url.c_str(), [=](HTTPRequestHandle request, int status, std::string rawBody) {
		const auto json = nlohmann::json::parse(rawBody);

		switch (status)
		{
			case 200:
			{
				std::string parseError;
				const auto playerInfo = KZ::API::FullPlayer::Deserialize(json, parseError);

				if (!playerInfo)
				{
					META_CONPRINTF("[KZ] Failed to fetch player from API: %s\n", parseError.c_str());
				}

				callback(playerInfo.value());
				break;
			}

			case 404:
			{
				if (!createIfNotExists)
				{
					player->languageService->PrintChat(true, false, "Player not found");
					break;
				}

				KZGlobalService::RegisterPlayer(player, []() {
				});

				break;
			}

			default:
			{
				std::string parseError;
				const auto error = KZ::API::Error::Deserialize(json, status, parseError);

				if (!error)
				{
					META_CONPRINTF("[KZ] Failed to parse API error: %s\n", parseError.c_str());
				}
				else
				{
					error->Report(player);
				}
			}
		}
	});
}

void KZGlobalService::FetchPlayer(KZPlayer *player, bool createIfNotExists, std::function<void(KZ::API::FullPlayer)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	u64 steamID = player->GetSteamId64();
	sprintf(&url.back(), "%llu", steamID);
	FetchPlayerImpl(player, url, createIfNotExists, callback);
}

void KZGlobalService::FetchPlayer(KZPlayer *player, u64 steamID, std::function<void(KZ::API::FullPlayer)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	sprintf(&url.back(), "%llu", steamID);
	FetchPlayerImpl(player, url, false, callback);
}

void KZGlobalService::FetchPlayer(KZPlayer *player, const char *name, std::function<void(KZ::API::FullPlayer)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	sprintf(&url.back(), "%s", name);
	FetchPlayerImpl(player, url, false, callback);
}

void KZGlobalService::FetchPreferences(KZPlayer *player, std::function<void(nlohmann::json)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	u64 steamID = player->GetSteamId64();
	sprintf(&url.back(), "%llu/preferences", steamID);

	g_HTTPManager.Get(url.c_str(), [player, callback](HTTPRequestHandle request, int status, std::string rawBody) {
		switch (status)
		{
			case 200:
			{
				const auto json = nlohmann::json::parse(rawBody);
				callback(json);
				break;
			}

			default:
			{
				const auto json = nlohmann::json::parse(rawBody);
				std::string parseError;
				const auto error = KZ::API::Error::Deserialize(json, status, parseError);

				if (!error)
				{
					META_CONPRINTF("[KZ] Failed to parse API error: %s\n", parseError.c_str());
				}
				else
				{
					error->Report();
				}

				break;
			}
		}
	});
}

void KZGlobalService::RegisterPlayer(KZPlayer *player, std::function<void()> callback)
{
	if (KZGlobalService::currentJWT == nullptr)
	{
		META_CONPRINTF("[KZ] Cannot register player with API because we are not authenticated.\n");
		return;
	}

	nlohmann::json payload;
	payload["name"] = player->GetName();
	payload["steam_id"] = player->GetSteamId64();
	payload["ip_address"] = player->GetIpAddress();

	std::vector<HTTPHeader> headers;
	headers.emplace_back("Authorization", (std::string("Bearer ") + KZGlobalService::currentJWT->c_str()));

	auto onResponse = [player](HTTPRequestHandle request, int status, std::string rawBody) {
		switch (status)
		{
			case 201:
			{
				KZGlobalService::FetchPlayer(player, false, [player](KZ::API::FullPlayer info) {
					player->languageService->PrintChat(true, false, "Display Hello", info.name.c_str());
					player->info = new KZ::API::FullPlayer(info);
				});

				break;
			}

			case 400:
			case 401:
			case 422:
			case 500:
			{
				const auto json = nlohmann::json::parse(rawBody);
				std::string parseError;
				const auto error = KZ::API::Error::Deserialize(json, status, parseError);

				if (!error)
				{
					META_CONPRINTF("[KZ] Failed to parse API error: %s\n", parseError.c_str());
				}
				else
				{
					error->Report();
				}

				break;
			}

			default:
			{
				META_CONPRINTF("[KZ] API returned unexpected status %d: %s\n", status, rawBody.c_str());
			}
		}
	};

	g_HTTPManager.Post((apiUrl + "/players").c_str(), payload.dump().c_str(), onResponse, &headers);
}
