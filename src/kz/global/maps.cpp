#include <string>

#include "vendor/nlohmann/json.hpp"

#include "kz_global.h"
#include "types/error.h"
#include "types/maps.h"
#include "utils/http.h"

void KZGlobalService::FetchMap(std::string mapIdentifier, std::function<void(KZ::API::Map)> callback)
{
	std::string url = KZGlobalService::apiUrl + "/maps/" + mapIdentifier;

	g_HTTPManager.Get(url.c_str(), [callback](HTTPRequestHandle request, int status, std::string rawBody) {
		switch (status)
		{
			case 200:
			{
				const auto json = nlohmann::json::parse(rawBody);
				std::string parseError;
				const auto map = KZ::API::Map::Deserialize(json, parseError);

				if (!map)
				{
					META_CONPRINTF("[KZ] Failed to parse API response: %s\n", parseError.c_str());
					return;
				}

				callback(map.value());
				break;
			}

			case 404:
			{
				META_CONPRINTF("[KZ] Map is not global.\n");
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
					return;
				}

				error->Report();
			}
		}
	});
}
