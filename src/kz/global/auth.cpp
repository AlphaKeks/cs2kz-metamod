#include <string>

#include "vendor/nlohmann/json.hpp"

#include "kz_global.h"
#include "types/error.h"
#include "utils/ctimer.h"
#include "version.h"

std::string *KZGlobalService::currentJWT = nullptr;

const f64 TWENTY_MINUTES = 60.0f * 20.0f;

f64 KZGlobalService::Auth()
{
	if (KZGlobalService::apiKey == nullptr)
	{
		META_CONPRINTF("[KZ] No API key found, can't authenticate!\n");
		return -1.0f;
	}

	nlohmann::json payload;
	payload["refresh_key"] = KZGlobalService::apiKey->c_str();
	payload["plugin_version"] = VERSION_STRING;

	g_HTTPManager.Post((apiUrl + "/servers/key").c_str(), payload.dump().c_str(), &OnAuth, nullptr);
	return TWENTY_MINUTES;
}

void KZGlobalService::OnAuth(HTTPRequestHandle request, int status, std::string rawBody)
{
	switch (status)
	{
		case 0:
		{
			META_CONPRINTF("[KZ] API is unreachable.\n");
			break;
		}

		case 201:
		{
			const nlohmann::json jwtResponse = nlohmann::json::parse(rawBody);
			META_CONPRINTF("[KZ] Authenticated with the API. JWT=%s\n", rawBody.c_str());
			KZGlobalService::currentJWT = new std::string(jwtResponse["access_key"]);
			break;
		}

		case 400:
		case 401:
		case 422:
		case 500:
		{
			const auto json = nlohmann::json::parse(rawBody);
			std::string parseError;
			const auto error = KZ::API::Error::Deserialize(json, status, parseError);

			if (!error)
			{
				META_CONPRINTF("[KZ] Failed to parse API error: %s\n", parseError.c_str());
			}
			else
			{
				error->Report();
			}

			break;
		}

		default:
		{
			META_CONPRINTF("[KZ] API returned unexpected status %d: %s\n", status, rawBody.c_str());
		}
	}
}
