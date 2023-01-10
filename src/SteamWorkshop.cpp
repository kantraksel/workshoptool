#include <set>
#include "nlohmann/json.hpp"
#include "HttpClient.h"
#include "Log.h"
#include "SteamWorkshop.h"

using SteamResponse = std::function<bool(const nlohmann::json& response)>;

static bool Post(Http::Client& client, const char* path, Http::Params& params, size_t itemCount, SteamResponse callback)
{
	auto result = client.Post(path, params);
	if (result.error != Http::NoError)
	{
		Log("Request error: {}", result.error);
		return false;
	}

	Log("Response status: {}", result.status);
	if (result.status == 200)
	{
		try
		{
			auto json = nlohmann::json::parse(result.body);
			auto& response = json["response"];

			int result = response["result"];
			int resultCount = response["resultcount"];

			if (result != 1)
			{
				Log("Invalid resource result: {}", result);
				Log("Location: {}", path);
			}
			else if (itemCount != resultCount)
			{
				Log("Unexpected resource count: expected {} got {}", itemCount, resultCount);
				Log("Location: {}", path);
			}
			else return callback(response);
		}
		catch (nlohmann::json::exception e)
		{
			Log("Invalid response: {}", e.what());
			Log("Location: {} ItemCount: {} Response:", path, itemCount);
			Log(result.body);
		}
	}

	return false;
}

static inline uintptr_t ToULL(const nlohmann::json& node)
{
	return std::stoull((std::string)node);
}

bool SteamWorkshop::ResolveCollections(Http::Client& client, std::set<uintptr_t> collections, AddonList& addons)
{
	Http::Params params;
	params.emplace("collectioncount", std::to_string(collections.size()));

	int index = 0;
	for (auto& item : collections)
	{
		params.emplace(std::format("publishedfileids[{}]", index), std::to_string(item));
		++index;
	}

	bool result = Post(client, szCollectionUrl, params, collections.size(), [&](const nlohmann::json& json)
		{
			auto& details = json["collectiondetails"];

			for (auto& collection : details)
			{
				uintptr_t cid = ToULL(collection["publishedfileid"]);
				if (collection["result"] != 1)
				{
					Log("Could not get details for collection {}", cid);
					continue;
				}

				int addonsAdded = 0;
				int addonsOmitted = 0;

				auto& children = collection["children"];
				for (auto& element : children)
				{
					uintptr_t id = ToULL(element["publishedfileid"]);
					auto result = addons.try_emplace(id);
					if (result.second) ++addonsAdded;
					else ++addonsOmitted;
				}

				Log("Resolved collection {}: {} addons added, {} duplicated addons omitted", cid, addonsAdded, addonsOmitted);
			}
			
			return true;
		});

	return result;
}

bool SteamWorkshop::ResolveAddons(Http::Client& client, AddonList& addons)
{
	Http::Params params;
	params.emplace("itemcount", std::to_string(addons.size()));

	int index = 0;
	for (auto& item : addons)
	{
		params.emplace(std::format("publishedfileids[{}]", index), std::to_string(item.first));
		++index;
	}

	int resolvedAddons = 0;

	bool result = Post(client, szAddonUrl, params, addons.size(), [&](const nlohmann::json& json)
		{
			auto& details = json["publishedfiledetails"];

			for (auto& element : details)
			{
				uintptr_t id = ToULL(element["publishedfileid"]);

				if (element["result"] != 1)
				{
					Log("Could not get details for addon {}", id);
					continue;
				}

				auto& addon = addons[id];

				auto file = std::filesystem::path((std::string)element["filename"]).filename();
				addon.name = element["title"];

				addon.url = element["file_url"];
				if (addon.url.empty())
				{
					Log("Addon '{}' has no download url - usually it means game's workshop is private!", addon.name);
					continue;
				}

				addon.file = file.string();
				if (addon.file.empty())
				{
					Log("Addon '{}' has no file name - setting file name to default value", addon.name);
					addon.file = std::format("{}.addon", id);
				}

				addon.size = element["file_size"];
				addon.download = true;
				++resolvedAddons;
			}

			return true;
		});

	Log("Addons ready to download: {}", resolvedAddons);
	if (resolvedAddons == 0)
		return false;

	return result;
}

AddonInfo::AddonInfo()
{
	size = 0;
	download = false;
}
