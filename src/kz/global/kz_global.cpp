#include <cstdio>
#include <string>

#include "vendor/nlohmann/json.hpp"

#include "../kz.h"
#include "../language/kz_language.h"
#include "kz/option/kz_option.h"
#include "kz_global.h"
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

internal SCMD_CALLBACK(Command_KzWho)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	auto callback = [player](KZ::API::FullPlayer info)
	{
		const char *name = info.name.c_str();
		const char *steamID = info.steamID.c_str();
		const char *isBanned = info.isBanned ? "" : "not ";

		player->languageService->PrintChat(true, false, "Display PlayerInfo", name, steamID, isBanned);
	};

	const char *playerIdentifier = args->Arg(1);

	if (playerIdentifier[0] == '\0')
	{
		KZGlobalService::FetchPlayer(player, callback);
	}
	else
	{
		KZGlobalService::FetchPlayer(player, playerIdentifier, callback);
	}

	return MRES_SUPERCEDE;
}

internal SCMD_CALLBACK(Command_KzPreferences)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	auto callback = [player](nlohmann::json json)
	{
		std::string text = json.dump();

		player->languageService->PrintChat(true, false, "View Preferences Response");
		player->languageService->PrintConsole(false, false, "Display Raw Preferences", text.c_str());
	};

	KZGlobalService::FetchPreferences(player, callback);

	return MRES_SUPERCEDE;
}

void KZGlobalService::RegisterCommands()
{
	scmd::RegisterCmd("kz_who", Command_KzWho);
	scmd::RegisterCmd("kz_preferences", Command_KzPreferences);
}
