#include <curl/curl.h>
#include "HttpClient.h"

using namespace Http;

static size_t DefaultResponseWrite(void* ptr, size_t _, size_t size, void* udata)
{
	auto& str = *reinterpret_cast<std::string*>(udata);

	str.append(std::string_view(static_cast<char*>(ptr), size));
	return size;
}

static size_t StreamResponseWrite(void* ptr, size_t _, size_t size, void* udata)
{
	auto& str = *reinterpret_cast<WriteCallback*>(udata);

	return str(static_cast<char*>(ptr), size) ? size : 0;
}

Client::Client()
{
	auto* curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_301);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/" LIBCURL_VERSION);
	}

	handle = curl;
}

Client::~Client()
{
	if (handle)
		curl_easy_cleanup(handle);
}

static void ConvertParams(const Params& params, std::string& outParams)
{
	for (auto& pair : params)
	{
		outParams.append(pair.first);
		outParams.push_back('=');
		outParams.append(pair.second);
		outParams.push_back('&');
	}
	outParams.pop_back();
	outParams.push_back(0);
}

Response Client::Post(const char* path, const Params& params)
{
	if (!handle) return Response{ CURLcode::CURLE_FAILED_INIT, 0 };
	Response response{ 0, 0 };

	std::string postParams;
	postParams.reserve(8 * 1048576);
	ConvertParams(params, postParams);

	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postParams.c_str());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, DefaultResponseWrite);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response.body);
	curl_easy_setopt(handle, CURLOPT_URL, path);
	auto res = curl_easy_perform(handle);

	response.error = res;
	if (res == CURLcode::CURLE_OK)
		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response.status);

	return response;
}

Response Client::Get(const char* path, WriteCallback writeCallback)
{
	if (!handle) return Response{ CURLcode::CURLE_FAILED_INIT, 0 };
	Response response{ 0, 0 };

	curl_easy_setopt(handle, CURLOPT_POST, 0L);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, StreamResponseWrite);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &writeCallback);
	curl_easy_setopt(handle, CURLOPT_URL, path);
	auto res = curl_easy_perform(handle);

	response.error = res;
	if (res == CURLcode::CURLE_OK)
		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response.status);

	return response;
}
