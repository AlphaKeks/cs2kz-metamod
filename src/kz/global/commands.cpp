#include "kz_global.h"
#include "kz/language/kz_language.h"
#include "kz/mode/kz_mode.h"
#include "utils/simplecmds.h"

static_function void DisplayMapInfo(const KZ::API::Map &map, KZPlayer *player)
{
	std::string sep;

	if (map.description)
	{
		sep = " | ";
	}

	std::string mappersText;

	for (size_t idx = 0; idx < map.mappers.size(); idx++)
	{
		mappersText += map.mappers[idx].name;

		if (idx != (map.mappers.size() - 1))
		{
			mappersText += ", ";
		}
	}

	player->languageService->PrintChat(true, false, "MapInfo", map.id, map.name, sep, map.description.value_or("").c_str(), map.workshop_id,
									   mappersText);
}

static_function void DisplayCourseInfo(const KZ::API::Map::Course &course, KZPlayer *player)
{
	std::string sep;

	if (course.description)
	{
		sep = " | ";
	}

	std::string mappersText;

	for (size_t idx = 0; idx < course.mappers.size(); idx++)
	{
		mappersText += course.mappers[idx].name;

		if (idx != (course.mappers.size() - 1))
		{
			mappersText += ", ";
		}
	}

	const char *mode = player->modeService->GetModeShortName();
	u8 tpTier = 0;
	const char *tpRanked = "";
	u8 proTier = 0;
	const char *proRanked = "";

	for (const KZ::API::Map::Course::Filter &filter : course.filters)
	{
		if ((filter.mode == KZ::API::Mode::Vanilla && strcmp(mode, "VNL") == 0)
			|| (filter.mode == KZ::API::Mode::Classic && strcmp(mode, "CKZ") == 0))
		{
			u8 tier = static_cast<u8>(filter.tier);

			if (filter.teleports)
			{
				tpTier = tier;

				switch (filter.ranked_status)
				{
					case KZ::API::Map::Course::Filter::RankedStatus::Never:
					{
						tpRanked = "unranked forever";
						break;
					}
					case KZ::API::Map::Course::Filter::RankedStatus::Unranked:
					{
						tpRanked = "unranked";
						break;
					}
					case KZ::API::Map::Course::Filter::RankedStatus::Ranked:
					{
						tpRanked = "ranked";
						break;
					}
				}
			}
			else
			{
				proTier = tier;

				switch (filter.ranked_status)
				{
					case KZ::API::Map::Course::Filter::RankedStatus::Never:
					{
						proRanked = "unranked forever";
						break;
					}
					case KZ::API::Map::Course::Filter::RankedStatus::Unranked:
					{
						proRanked = "unranked";
						break;
					}
					case KZ::API::Map::Course::Filter::RankedStatus::Ranked:
					{
						proRanked = "ranked";
						break;
					}
				}
			}
		}
	}

	player->languageService->PrintChat(true, false, "CourseInfo", course.id, course.name.value_or("unknown").c_str(), sep,
									   course.description.value_or("").c_str(), mappersText, mode, tpTier, tpRanked, proTier, proRanked);
}

static_function SCMD_CALLBACK(Command_KzMapInfo)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	const char *mapName = args->Arg(1);

	if (mapName[0] != '\0')
	{
		auto callback = [player](std::optional<KZ::API::Map> map)
		{
			if (!map)
			{
				player->languageService->PrintChat(true, false, "MapNotGlobal");
				return;
			}

			DisplayMapInfo(map.value(), player);
		};

		player->globalService->FetchMap(mapName, callback);
	}
	else if (KZGlobalService::currentMap.has_value())
	{
		DisplayMapInfo(KZGlobalService::currentMap.value(), player);
	}
	else
	{
		player->languageService->PrintChat(true, false, "MapNotGlobal");
	}

	return MRES_SUPERCEDE;
}

static_function SCMD_CALLBACK(Command_KzCourseInfo)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	const char *courseName = args->Arg(1);

	if (!KZGlobalService::currentMap.has_value())
	{
		player->languageService->PrintChat(true, false, "MapNotGlobal");
		return MRES_SUPERCEDE;
	}

	if (courseName[0] != '\0')
	{
		bool found = false;

		for (const KZ::API::Map::Course &course : KZGlobalService::currentMap->courses)
		{
			if (course.name.has_value() && V_stricmp(course.name->c_str(), courseName) == 0)
			{
				found = true;
				DisplayCourseInfo(course, player);
				break;
			}
		}

		if (!found)
		{
			player->languageService->PrintChat(true, false, "CourseNotFound");
		}
	}
	else
	{
		// TODO: figure out on which course the player last had a running timer on
		player->languageService->PrintChat(true, false, "CourseNotFound");
	}

	return MRES_SUPERCEDE;
}

void KZGlobalService::RegisterCommands()
{
	scmd::RegisterCmd("kz_mapinfo", Command_KzMapInfo);
	scmd::RegisterCmd("kz_minfo", Command_KzMapInfo);
	scmd::RegisterCmd("kz_courseinfo", Command_KzCourseInfo);
	scmd::RegisterCmd("kz_cinfo", Command_KzCourseInfo);
}
