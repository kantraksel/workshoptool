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

	Log("Request status: {}", result.status);
	if (result.status == 200)
	{
		try
		{
			auto json = nlohmann::json::parse(result.body);
			auto& response = json["response"];

			int result = response["result"];
			int resultCount = response["resultcount"];

			if (result == 1 && itemCount == resultCount)
			{
				return callback(response);
			}
			else Log("Response: result({}) resultcount({})", result, resultCount);
		}
		catch (nlohmann::json::exception e)
		{
			Log("Invalid response: {}", result.body);
		}
	}

	return false;
}

static inline uintptr_t ToULL(const nlohmann::json& node)
{
	return std::stoull((std::string)node);
}

bool SteamWorkshop::ResolveCollection(Http::Client& client, uintptr_t collectionId, AddonList& addons)
{
	auto szCollectionId = std::to_string(collectionId);
	Log("Resolving collection: {}", szCollectionId);

	Http::Params params;
	params.emplace("collectioncount", "1");
	params.emplace("publishedfileids[0]", szCollectionId);

	bool result = Post(client, szCollectionUrl, params, 1, [&](const nlohmann::json& json)
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

				auto& children = collection["children"];
				for (auto& element : children)
				{
					uintptr_t id = ToULL(element["publishedfileid"]);
					addons.try_emplace(id);
					Log("Adding addon from collection: {}", id);
				}
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

	bool result = Post(client, szAddonUrl, params, addons.size(), [&](const nlohmann::json& json)
		{
			auto& details = json["publishedfiledetails"];

			for (auto& element : details)
			{
				if (element["result"] != 1)
				{
					Log("Could not get details for addon {}", (std::string)element["publishedfileid"]);
					continue;
				}

				uintptr_t id = ToULL(element["publishedfileid"]);
				auto& addon = addons[id];

				auto file = std::filesystem::path((std::string)element["filename"]).filename();

				addon.file = file.string();
				addon.url = element["file_url"];
				addon.size = element["file_size"];
			}

			return true;
		});

	return result;
}

