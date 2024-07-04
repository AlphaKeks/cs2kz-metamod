#include <atomic>
#include <string>

#include "kz_global.h"
#include "utils/ctimer.h"
#include "utils/http.h"

extern ISteamHTTP *g_pHTTP;
std::atomic<bool> KZGlobalService::authTimerInitialized = false;

f64 KZGlobalService::Heartbeat()
{
	g_HTTPManager.Get(apiUrl.c_str(), &OnHeartbeat);
	return 30.0f;
}

void KZGlobalService::OnHeartbeat(HTTPRequestHandle request, int status, std::string rawBody)
{
	switch (status)
	{
		case 0:
		{
			META_CONPRINTF("[KZ] API is unreachable.\n");
			break;
		}

		case 200:
		{
			META_CONPRINTF("[KZ] API is healthy %s\n", rawBody.c_str());

			if (!authTimerInitialized)
			{
				authTimerInitialized = true;
				META_CONPRINTF("[KZ] Initializing auth flow\n");
				StartTimer(Auth, true, true);
			}

			break;
		}

		default:
		{
			META_CONPRINTF("[KZ] API returned unexpected status on healthcheck: %d (%s)\n", status, rawBody.c_str());
		}
	}
}
