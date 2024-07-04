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
				const auto map = KZ::API::Map::Deserialize(json);
				callback(map);
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
				const auto error = KZ::API::Error::Deserialize(json, status);
				error.Report();
			}
		}
	});
}
