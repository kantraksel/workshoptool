#include <cstdio>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <list>
#include <queue>
#include <string>

#include "HttpClient.h"
#include "nlohmann/json.hpp"
#include "version.h"

const char* szCollectionUrl = "https://api.steampowered.com/ISteamRemoteStorage/GetCollectionDetails/v1";
const char* szAddonUrl = "https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1";

using PostResponse = std::function<void(const std::string& body)>;

struct StringTuplet
{
	std::string first;
	std::string second;
};

static void Log(const std::string& str)
{
	std::cout << str << std::endl;
}

template <class... Args>
static inline void Log(const std::_Fmt_string<Args...> _Fmt, Args&&... _Args)
{
	Log(std::vformat(_Fmt._Str, std::make_format_args(_Args...)));
}

void ReadFile(const char* name, std::list<uintptr_t>& ids, uintptr_t& collectionId);
void ResolveCollection(Http::Client& client, uintptr_t collection, std::list<uintptr_t>& addons);
void ResolveAddons(Http::Client& client, const std::list<uintptr_t>& addons, std::queue<StringTuplet>& downloadQueue);
void DownloadAddons(Http::Client& client, std::queue<StringTuplet>& downloadQueue);

int Exit(const char* msg, int code = 0)
{
	Log(msg);

#if not HEADLESS
	Log("Press any key to exit");
	std::getchar();
#endif

	return code;
}

inline void FailExit(const char* msg)
{
	std::exit(Exit(msg, 1));
}

int main()
{
	Log("WorkShop Tool " VERSION);

	Http::Client client;
	std::list<uintptr_t> addons;

	uintptr_t collection = 0;
	ReadFile("steam.wst", addons, collection);
	ResolveCollection(client, collection, addons);

	if (addons.empty())
		return Exit("No addons to download");

	Log("Addons in queue: {}", addons.size());
	Log("Resolving urls...");

	std::queue<StringTuplet> downloadQueue;
	ResolveAddons(client, addons, downloadQueue);

	if (downloadQueue.empty())
		return Exit("No addons resolved");

	Log("Resolved urls: {}", downloadQueue.size());

	std::filesystem::create_directory("wst");
	DownloadAddons(client, downloadQueue);

	return Exit("Done");
}

bool Post(Http::Client& client, const char* path, Http::Params& params, PostResponse response);

void ResolveCollection(Http::Client& client, uintptr_t collection, std::list<uintptr_t>& addons)
{
	if (collection != 0)
	{
		Log("Resolving collection: " + std::to_string(collection));

		Http::Params params;
		params.emplace("collectioncount", "1");
		params.emplace("publishedfileids[0]", std::to_string(collection));

		bool result = Post(client, szCollectionUrl, params, [&](const std::string& body) {
			try
			{
				auto json = nlohmann::json::parse(body)["response"];
				int result = json["result"];
				int resultcount = json["resultcount"];
				if (result == resultcount == 1)
				{
					auto& details = json["collectiondetails"][0]["children"];

					for (auto& element : details)
					{
						std::string sid = element["publishedfileid"];
						uintptr_t id = std::stoull(sid);
						addons.push_back(id);
						Log("Adding addon from collection: {}", id);
					}
				}
				else Log("Response: result({}) resultcount({})", result, resultcount);
			}
			catch (...)
			{
				Log("Invalid response: {}", body);
			}
			});

		if (!result)
			FailExit("");
	}
	else Log("No collection specified");
}

void ResolveAddons(Http::Client& client, const std::list<uintptr_t>& addons, std::queue<StringTuplet>& downloadQueue)
{
	Http::Params params;
	params.emplace("itemcount", std::to_string(addons.size()));

	int index = 0;
	for (auto& element : addons)
	{
		params.emplace(std::format("publishedfileids[{}]", index), std::to_string(element));
		++index;
	}

	bool result = Post(client, szAddonUrl, params, [&](const std::string& body) {
		try
		{
			auto json = nlohmann::json::parse(body)["response"];
			int result = json["result"];
			int resultcount = json["resultcount"];
			if (result == 1 && resultcount == addons.size())
			{
				auto& details = json["publishedfiledetails"];

				for (auto& element : details)
				{
					result = element["result"];
					if (result != 1)
					{
						Log("Could not get details for addon {}", (std::string)element["publishedfileid"]);
						continue;
					}

					auto path = std::filesystem::path((std::string)element["filename"]);
					downloadQueue.push({ element["file_url"], path.filename().string() });
				}
			}
			else Log("Response: result({}) resultcount({})", result, resultcount);
		}
		catch (...)
		{
			Log("Invalid response: {}", body);
		}
		});

	if (!result)
		FailExit("");
}

void DownloadAddons(Http::Client& client, std::queue<StringTuplet>& downloadQueue)
{
	std::filesystem::current_path("wst/");
	std::ofstream os;

	while (!downloadQueue.empty())
	{
		StringTuplet entry = downloadQueue.front();
		downloadQueue.pop();

		if (os.open(entry.second), !os)
		{
			Log("Could not open {} Skipping...", entry.second);
			continue;
		}

		Log("Downloading {}", entry.second);

		auto res = client.Get(
			entry.first.c_str(),
			[&](const char* data, size_t data_length) {
				os.write(data, data_length);
				return (bool)os;
			}
		);

		os.close();
		if (res.error == Http::NoError && res.status == 200) Log("Download completed!");
		else Log("Download failed!");
	}
}

void ReadFile(const char* name, std::list<uintptr_t>& ids, uintptr_t& collectionId)
{
	uintptr_t entry;
	std::ifstream is(name);

	while (is >> entry)
	{
		ids.push_back(entry);
	}

	//Collection is always one and first at the list
	//will change in future to support collection list
	auto i = ids.begin();
	if (i != ids.end())
	{
		collectionId = *i;
		++i;
		ids.pop_front();
	}
}

bool Post(Http::Client& client, const char* path, Http::Params& params, PostResponse response)
{
	auto result = client.Post(path, params);
	if (result.error != Http::NoError)
	{
		Log("Request error: {}", result.error);
		return false;
	}

	Log("Request status: {}", result.status);
	if (result.status == 200)
	{
		response(result.body);
		return true;
	}

	return false;
}
