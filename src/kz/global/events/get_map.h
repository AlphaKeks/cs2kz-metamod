#pragma once

namespace KZ::API::Events
{
	template<typename T>
	struct GetMap
	{
		GetMap() = default;

		GetMap(const char *name) : map_identifier(std::string(name)) {}

		GetMap(u16 id) : map_identifier(id) {}

	private:
		T map_identifier {};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GetMap, map_identifier)
	};
}; // namespace KZ::API::Events
