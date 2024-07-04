#include <cstdio>
#include <string>

#include "vendor/nlohmann/json.hpp"

#include "../kz.h"
#include "../language/kz_language.h"
#include "kz/option/kz_option.h"
#include "kz_global.h"
#include "types/maps.h"
#include "utils/ctimer.h"
#include "utils/simplecmds.h"

std::string KZGlobalService::apiUrl;
std::string *KZGlobalService::apiKey = nullptr;
KZ::API::Map *KZGlobalService::currentMap = nullptr;

void KZGlobalService::Init()
{
	KZGlobalService::apiUrl = std::string(KZOptionService::GetOptionStr("apiUrl", "https://api.cs2kz.org"));
	META_CONPRINTF("[KZ] Registered API URL: `%s`\n", KZGlobalService::apiUrl.c_str());

	const char *apiKey = KZOptionService::GetOptionStr("apiKey", "");

	if (apiKey[0] != '\0')
	{
		KZGlobalService::apiKey = new std::string(apiKey);
	}

	StartTimer(Heartbeat, true, true);
}

internal SCMD_CALLBACK(Command_KzProfile)
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
		KZGlobalService::FetchPlayer(player, false, callback);
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

internal SCMD_CALLBACK(Command_KzMapInfo)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	auto currentMap = KZGlobalService::currentMap;

	if (currentMap == nullptr)
	{
		player->languageService->PrintChat(true, false, "CurrentMapNotGlobal");
	}
	else
	{
		std::string sep;

		if (!currentMap->description.empty())
		{
			sep = " | ";
		}

		std::string mappers;

		for (size_t idx = 0; idx < currentMap->mappers.size(); idx++)
		{
			mappers += currentMap->mappers[idx].name;

			if (idx != (currentMap->mappers.size() - 1))
			{
				mappers += ", ";
			}
		}

		player->languageService->PrintChat(true, false, "CurrentMap", currentMap->id, currentMap->name, sep.c_str(), currentMap->description.c_str(),
										   currentMap->workshopID, mappers.c_str());
	}

	return MRES_SUPERCEDE;
}

void KZGlobalService::RegisterCommands()
{
	scmd::RegisterCmd("kz_profile", Command_KzProfile);
	scmd::RegisterCmd("kz_preferences", Command_KzPreferences);
	scmd::RegisterCmd("kz_mapinfo", Command_KzMapInfo);
	scmd::RegisterCmd("kz_minfo", Command_KzMapInfo);
}
