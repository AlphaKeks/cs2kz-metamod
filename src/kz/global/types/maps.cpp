#include "vendor/nlohmann/json.hpp"

#include "kz/language/kz_language.h"
#include "maps.h"

namespace KZ::API
{
	std::optional<Map> Map::Deserialize(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_object())
		{
			parseError = "Map returned by API is not a JSON object.";
			return std::nullopt;
		}

		if (!json.contains("id"))
		{
			parseError = "Map returned by API does not have an ID.";
			return std::nullopt;
		}

		auto id = json["id"];

		if (!json.contains("name"))
		{
			parseError = "Map returned by API does not have a name.";
			return std::nullopt;
		}

		auto name = json["name"];
		std::optional<std::string> description = std::nullopt;

		if (json.contains("description"))
		{
			if (!json["description"].is_string())
			{
				parseError = "Map description is not a string.";
				return std::nullopt;
			}

			description = json["description"];
		}

		if (!json.contains("global_status"))
		{
			parseError = "Map returned by API does not have a global status.";
			return std::nullopt;
		}

		auto globalStatus = Map::DeserializeGlobalStatus(json["global_status"], parseError);

		if (!globalStatus)
		{
			return std::nullopt;
		}

		if (!json.contains("workshop_id"))
		{
			parseError = "Map returned by API does not have a Workshop ID.";
			return std::nullopt;
		}

		auto workshopID = json["workshop_id"];

		if (!json.contains("checksum"))
		{
			parseError = "Map returned by API does not contain a checksum.";
			return std::nullopt;
		}

		auto checksum = json["checksum"];

		if (!json.contains("created_on"))
		{
			parseError = "Map returned by API does not contain a creation date.";
			return std::nullopt;
		}

		auto createdOn = json["created_on"];

		std::vector<Player> mappers;

		if (json.contains("mappers"))
		{
			if (!json["mappers"].is_array())
			{
				parseError = "Map's mappers field is not an array.";
				return std::nullopt;
			}

			for (const auto &mapper : json["mappers"])
			{
				auto parsedMapper = Player::Deserialize(mapper, parseError);

				if (!parsedMapper)
				{
					return std::nullopt;
				}

				mappers.push_back(parsedMapper.value());
			}
		}

		std::vector<Course> courses;

		if (json.contains("courses"))
		{
			if (!json["courses"].is_array())
			{
				parseError = "Map's courses field is not an array.";
				return std::nullopt;
			}

			for (const auto &course : json["courses"])
			{
				auto parsedCourse = Course::Deserialize(course, parseError);

				if (!parsedCourse)
				{
					return std::nullopt;
				}

				courses.push_back(parsedCourse.value());
			}
		}

		return Map {
			.id = id,
			.name = name,
			.description = description,
			.globalStatus = globalStatus.value(),
			.workshopID = workshopID,
			.checksum = checksum,
			.mappers = mappers,
			.courses = courses,
			.createdOn = createdOn,
		};
	}

	std::optional<Map::GlobalStatus> Map::DeserializeGlobalStatus(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_string())
		{
			parseError = "global status is not a string.";
			return std::nullopt;
		}

		std::string globalStatus = json;

		if (globalStatus == "global")
		{
			return Map::GlobalStatus::GLOBAL;
		}
		else if (globalStatus == "in_testing")
		{
			return Map::GlobalStatus::IN_TESTING;
		}
		else if (globalStatus == "not_global")
		{
			return Map::GlobalStatus::NOT_GLOBAL;
		}
		else
		{
			parseError = "unknown global status";
			return std::nullopt;
		}
	}

	void Map::Display(KZPlayer *player) const
	{
		std::string sep;

		if (description)
		{
			sep = " | ";
		}

		std::string mappersText;

		for (size_t idx = 0; idx < mappers.size(); idx++)
		{
			mappersText += mappers[idx].name;

			if (idx != (mappers.size() - 1))
			{
				mappersText += ", ";
			}
		}

		player->languageService->PrintChat(true, false, "CurrentMap", id, name, sep.c_str(), description.value_or("").c_str(), workshopID,
										   mappersText.c_str());
	}

	std::optional<Course> Course::Deserialize(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_object())
		{
			parseError = "Course returned by the API is not a JSON object.";
			return std::nullopt;
		}

		if (!json.contains("id"))
		{
			parseError = "Course returned by the API does not have an ID.";
			return std::nullopt;
		}

		if (!json["id"].is_number_unsigned())
		{
			parseError = "Course ID is not an integer.";
			return std::nullopt;
		}

		auto id = json["id"];

		std::optional<std::string> name = std::nullopt;

		if (json.contains("name"))
		{
			if (!json["name"].is_string())
			{
				parseError = "Map name is not a string";
				return std::nullopt;
			}

			name = json["name"];
		}

		std::optional<std::string> description = std::nullopt;

		if (json.contains("description"))
		{
			if (!json["description"].is_string())
			{
				parseError = "Course description is not a string.";
				return std::nullopt;
			}

			description = json["description"];
		}

		std::vector<Player> mappers;

		if (json.contains("mappers"))
		{
			if (!json["mappers"].is_array())
			{
				parseError = "Courses's mappers field is not an array.";
				return std::nullopt;
			}

			for (const auto &mapper : json["mappers"])
			{
				auto parsedMapper = Player::Deserialize(mapper, parseError);

				if (!parsedMapper)
				{
					return std::nullopt;
				}

				mappers.push_back(parsedMapper.value());
			}
		}

		std::vector<Filter> filters;

		if (json.contains("filters"))
		{
			if (!json["filters"].is_array())
			{
				parseError = "Courses's filters field is not an array.";
				return std::nullopt;
			}

			for (const auto &filter : json["filters"])
			{
				auto parsedFilter = Filter::Deserialize(filter, parseError);

				if (!parsedFilter)
				{
					return std::nullopt;
				}

				filters.push_back(parsedFilter.value());
			}
		}

		return Course {
			.id = id,
			.name = name,
			.description = description,
			.mappers = mappers,
			.filters = filters,
		};
	}

	std::optional<Filter> Filter::Deserialize(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_object())
		{
			parseError = "course filter returned by API is not a JSON object.";
			return std::nullopt;
		}

		if (!json.contains("id"))
		{
			parseError = "course filter does not have an ID.";
			return std::nullopt;
		}

		if (!json["id"].is_number_unsigned())
		{
			parseError = "Filter ID is not an integer.";
			return std::nullopt;
		}

		auto id = json["id"];
		auto mode = DeserializeMode(json, parseError);

		if (!mode)
		{
			return std::nullopt;
		}

		if (!json.contains("teleports"))
		{
			parseError = "course filter does not have a `teleports` field.";
			return std::nullopt;
		}

		if (!json["teleports"].is_boolean())
		{
			parseError = "Filter `teleports` field is not a boolean.";
			return std::nullopt;
		}

		auto teleports = json["teleports"];
		auto tier = DeserializeTier(json, parseError);

		if (!tier)
		{
			return std::nullopt;
		}

		auto rankedStatus = Filter::DeserializeRankedStatus(json, parseError);

		if (!rankedStatus)
		{
			return std::nullopt;
		}

		std::optional<std::string> notes = std::nullopt;

		if (json.contains("notes"))
		{
			if (!json["notes"].is_string())
			{
				parseError = "Filter `notes` field is not a string.";
				return std::nullopt;
			}

			notes = json["notes"];
		}

		return Filter {
			.id = id,
			.mode = mode.value(),
			.teleports = teleports,
			.tier = tier.value(),
			.rankedStatus = rankedStatus.value(),
			.notes = notes,
		};
	}

	std::optional<Filter::RankedStatus> Filter::DeserializeRankedStatus(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_string())
		{
			parseError = "ranked status is not a string";
			return std::nullopt;
		}

		std::string rankedStatus = json;

		if (rankedStatus == "never")
		{
			return Filter::RankedStatus::NEVER;
		}
		else if (rankedStatus == "unranked")
		{
			return Filter::RankedStatus::UNRANKED;
		}
		else if (rankedStatus == "ranked")
		{
			return Filter::RankedStatus::RANKED;
		}
		else
		{
			parseError = "unknown ranked status";
			return std::nullopt;
		}
	}
} // namespace KZ::API
