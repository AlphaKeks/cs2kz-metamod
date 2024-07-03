/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023-2024 Source2ZE
 * Original code from D2Lobby2
 * Copyright (C) 2023 Nicholas Hastings
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2.0 or later, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you are also granted permission to link the code
 * of this program (as well as its derivative works) to "Dota 2," the
 * "Source Engine, and any Game MODs that run on software by the Valve Corporation.
 * You must obey the GNU General Public License in all respects for all other
 * code used.  Additionally, this exception is granted to all derivative works.
 */

#include "http.h"
#include "common.h"
#include <string>

ISteamHTTP* g_pHTTP = nullptr;
HTTPManager g_HTTPManager;

#undef strdup

HTTPManager::TrackedRequest::TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, std::string strUrl, std::string strText, CompletedCallback callback)
{
	m_hHTTPReq = hndl;
	m_CallResult.SetGameserverFlag();
	m_CallResult.Set(hCall, this, &TrackedRequest::OnHTTPRequestCompleted);

	m_strUrl = strUrl;
	m_strText = strText;
	m_callback = callback;

	g_HTTPManager.m_PendingRequests.push_back(this);
}

HTTPManager::TrackedRequest::~TrackedRequest()
{
	for (auto e = g_HTTPManager.m_PendingRequests.begin(); e != g_HTTPManager.m_PendingRequests.end(); ++e)
	{
		if (*e == this)
		{
			g_HTTPManager.m_PendingRequests.erase(e);
			break;
		}
	}
}

void HTTPManager::TrackedRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t* arg, bool bFailed)
{
	if (bFailed)
	{
		Msg("HTTP request to %s failed with status code %i\n", m_strUrl.c_str(), arg->m_eStatusCode);
	}
	else
	{
		uint32 size;
		g_pHTTP->GetHTTPResponseBodySize(arg->m_hRequest, &size);

		uint8* response = new uint8[size + 1];
		g_pHTTP->GetHTTPResponseBodyData(arg->m_hRequest, response, size);
		response[size] = 0; // Add null terminator

		m_callback(arg->m_hRequest, arg->m_eStatusCode, (char*)response);

		delete[] response;
	}

	if (g_pHTTP)
		g_pHTTP->ReleaseHTTPRequest(arg->m_hRequest);

	delete this;
}

void HTTPManager::Get(const char* pszUrl, CompletedCallback callback, std::vector<HTTPHeader>* headers)
{
	GenerateRequest(k_EHTTPMethodGET, pszUrl, "", callback, headers);
}

void HTTPManager::Post(const char* pszUrl, const char* pszText, CompletedCallback callback, std::vector<HTTPHeader>* headers)
{
	GenerateRequest(k_EHTTPMethodPOST, pszUrl, pszText, callback, headers);
}

void HTTPManager::GenerateRequest(EHTTPMethod method, const char* pszUrl, const char* pszText, CompletedCallback callback, std::vector<HTTPHeader>* headers)
{
	// Msg("Sending HTTP request to `%s`:\n```\n%s\n```\n", pszUrl, pszText);
	auto hReq = g_pHTTP->CreateHTTPRequest(method, pszUrl);
	int size = strlen(pszText);
	//Msg("HTTP request: %p\n", hReq);

	if (method == k_EHTTPMethodPOST && !g_pHTTP->SetHTTPRequestRawPostBody(hReq, "application/json", (uint8*)pszText, size))
	{
		Msg("Failed to SetHTTPRequestRawPostBody\n");
		return;
	}

	// Prevent HTTP error 411 (probably not necessary?)
	//g_pHTTP->SetHTTPRequestHeaderValue(hReq, "Content-Length", std::to_string(size).c_str());

	if (headers != nullptr)
	{
		for (HTTPHeader header : *headers)
			g_pHTTP->SetHTTPRequestHeaderValue(hReq, header.GetName(), header.GetValue());
	}

	SteamAPICall_t hCall;

	if (!g_pHTTP->SendHTTPRequest(hReq, &hCall))
	{
		Msg("failed to send http request\n");
	}

	new TrackedRequest(hReq, hCall, pszUrl, pszText, callback);
}
