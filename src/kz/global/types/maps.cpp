#include "vendor/nlohmann/json.hpp"
#include "maps.h"

namespace KZ::API
{
	Map Map::Deserialize(const nlohmann::json &json)
	{
		Map map;
		json.at("id").get_to(map.id);
		json.at("name").get_to(map.name);
		map.description = json.value("description", "");
		map.globalStatus = Map::DeserializeGlobalStatus(json);
		json.at("workshop_id").get_to(map.workshopID);
		json.at("checksum").get_to(map.checksum);
		json.at("created_on").get_to(map.createdOn);

		for (const auto &mapper : json.at("mappers"))
		{
			map.mappers.push_back(Player::Deserialize(mapper));
		}

		for (const auto &course : json.at("courses"))
		{
			map.courses.push_back(Course::Deserialize(course));
		}

		return map;
	}

	Map::GlobalStatus Map::DeserializeGlobalStatus(const nlohmann::json &json)
	{
		std::string globalStatus;
		json.at("global_status").get_to(globalStatus);

		if (globalStatus == "global")
		{
			return Map::GlobalStatus::GLOBAL;
		}
		else if (globalStatus == "in_testing")
		{
			return Map::GlobalStatus::IN_TESTING;
		}
		else
		{
			return Map::GlobalStatus::NOT_GLOBAL;
		}
	}

	Course Course::Deserialize(const nlohmann::json &json)
	{
		Course course;

		json.at("id").get_to(course.id);
		course.name = json.value("name", "");
		course.description = json.value("description", "");

		for (const auto &mapper : json.at("mappers"))
		{
			course.mappers.push_back(Player::Deserialize(mapper));
		}

		for (const auto &filter : json.at("filters"))
		{
			course.filters.push_back(Filter::Deserialize(filter));
		}

		return course;
	}

	Filter Filter::Deserialize(const nlohmann::json &json)
	{
		Filter filter;

		json.at("id").get_to(filter.id);
		filter.mode = DeserializeMode(json);
		json.at("teleports").get_to(filter.teleports);
		filter.tier = DeserializeTier(json);
		filter.rankedStatus = Filter::DeserializeRankedStatus(json);
		filter.notes = json.value("notes", "");

		return filter;
	}

	Filter::RankedStatus Filter::DeserializeRankedStatus(const nlohmann::json &json)
	{
		std::string rankedStatus;
		json.at("ranked_status").get_to(rankedStatus);

		// TODO
		return Filter::RankedStatus::NEVER;
	}
} // namespace KZ::API
