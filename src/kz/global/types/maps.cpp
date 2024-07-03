#include "vendor/nlohmann/json.hpp"
#include "maps.h"

namespace KZ::API
{
	Map Map::Deserialize(const nlohmann::json &json)
	{
		Map map;
		json.at("id").get_to(map.id);
		json.at("name").get_to(map.name);

		auto description = json.value("description", "");

		if (description != "")
		{
			map.description = new std::string(description);
		}

		json.at("global_status").get_to(map.globalStatus);
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

	Course Course::Deserialize(const nlohmann::json &json)
	{
		Course course;

		json.at("id").get_to(course.id);

		auto name = json.value("name", "");

		if (name != "")
		{
			course.name = new std::string(name);
		}

		auto description = json.value("description", "");

		if (description != "")
		{
			course.description = new std::string(description);
		}

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
		json.at("mode").get_to(filter.mode);
		json.at("teleports").get_to(filter.teleports);
		json.at("tier").get_to(filter.tier);
		json.at("ranked_status").get_to(filter.rankedStatus);

		auto notes = json.value("notes", "");

		if (notes != "")
		{
			filter.notes = new std::string(notes);
		}

		return filter;
	}
} // namespace KZ::API
