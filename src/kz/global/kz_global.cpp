#include <cstdio>
#include <string>

#include "vendor/nlohmann/json.hpp"

#include "../kz.h"
#include "kz/option/kz_option.h"
#include "kz_global.h"
#include "types/players.h"
#include "utils/ctimer.h"
#include "utils/http.h"
#include "utils/simplecmds.h"

extern ISteamHTTP *g_pHTTP;
CSteamGameServerAPIContext g_steamAPI;
std::string KZGlobalService::apiUrl;

void KZGlobalService::Init()
{
	KZGlobalService::apiUrl = std::string(KZOptionService::GetOptionStr("apiUrl", "https://api.cs2kz.org"));
	META_CONPRINTF("[KZ] Registered API URL: `%s`\n", KZGlobalService::apiUrl.c_str());

	StartTimer(Heartbeat, true, true);
}

f64 KZGlobalService::Heartbeat()
{
	if (!g_pHTTP)
	{
		g_steamAPI.Init();
		g_pHTTP = g_steamAPI.SteamHTTP();
	}

	g_HTTPManager.Get(apiUrl.c_str(), &OnHeartbeat);

	return 30.0f;
}

void KZGlobalService::OnHeartbeat(HTTPRequestHandle request, int status, std::string rawBody)
{
	switch (status)
	{
		case 0:
			META_CONPRINTF("[KZ] API is unreachable.\n");
			break;

		case 200:
			META_CONPRINTF("[KZ] API is healthy %s\n", rawBody.c_str());
			break;

		default:
			META_CONPRINTF("[KZ] API returned unexpected status on healthcheck: %d (%s)\n", status, rawBody.c_str());
	}
}

void KZGlobalService::FetchPlayer(KZPlayer *player, std::function<void(KZ::API::Player)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/players/:";
	u64 steamID = player->GetSteamId64();
	sprintf(&url.back(), "%llu", steamID);

	auto on_response = [callback](HTTPRequestHandle request, int status, std::string rawBody)
	{
		switch (status)
		{
			case 200:
				// good
				break;

			case 404:
				META_CONPRINTF("[KZ] The API doesn't know you :( this is a bug!\n", status, rawBody.c_str());
				return;

			default:
				META_CONPRINTF("[KZ] Failed to fetch your information (%d): %s\n", status, rawBody.c_str());
				return;
		}

		const auto json = nlohmann::json::parse(rawBody);
		const auto playerInfo = KZ::API::Player::Deserialize(json);

		callback(playerInfo);
	};

	g_HTTPManager.Get(url.c_str(), on_response);
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

			case 404:
				player->PrintChat(true, true, "The API doesn't know you :( this is a bug!\n", status, rawBody.c_str());
				return;

			default:
				player->PrintChat(true, true, "Failed to fetch your information (%d): %s\n", status, rawBody.c_str());
				return;
		}

		const auto json = nlohmann::json::parse(rawBody);

		callback(json);
	};

	g_HTTPManager.Get(url.c_str(), on_response);
}

internal SCMD_CALLBACK(Command_KzWho)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	auto callback = [player](KZ::API::Player playerInfo)
	{
		const char *name = playerInfo.name.c_str();
		const char *steamID = playerInfo.steamID.c_str();
		const char *bannedText = playerInfo.isBanned ? " " : " not ";

		player->PrintChat(true, true, "Hello, %s! Your SteamID is %s and you are currently%sbanned.\n", name, steamID, bannedText);
	};

	KZGlobalService::FetchPlayer(player, callback);

	return MRES_SUPERCEDE;
}

internal SCMD_CALLBACK(Command_KzPreferences)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	auto callback = [player](nlohmann::json json)
	{
		std::string text = json.dump();

		player->PrintConsole(false, true, "Your preferences:\n%s\n", text.c_str());
	};

	KZGlobalService::FetchPreferences(player, callback);

	return MRES_SUPERCEDE;
}

void KZGlobalService::RegisterCommands()
{
	scmd::RegisterCmd("kz_who", Command_KzWho);
	scmd::RegisterCmd("kz_preferences", Command_KzPreferences);
}
