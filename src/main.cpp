#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

#include "HttpClient.h"
#include "Log.h"
#include "SteamWorkshop.h"
#include "version.h"

constexpr const char* szCollectionList = "collections.txt";
constexpr const char* szAddonList = "addons.txt";
constexpr const char* szOutputDir = "addons";

void LoadCollections(Http::Client& client, AddonList& addons);
void LoadStandaloneAddons(AddonList& addons);
void DownloadAddons(Http::Client& client, AddonList& addons);

int Exit(const char* msg, int code = 0)
{
	Log(msg);

#if not HEADLESS
	Log("Press any key to exit");
	std::getchar();
#endif

	return code;
}

inline void FailExit(const char* msg = "")
{
	std::exit(Exit(msg, 1));
}

int main()
{
	Log("WorkShop Tool " VERSION);

	Http::Client client;
	AddonList addons;

	LoadStandaloneAddons(addons);
	LoadCollections(client, addons);

	if (addons.empty())
		return Exit("No addons to download");

	Log("Addons in queue: {}", addons.size());
	Log("Downloading addon info...");

	if (!SteamWorkshop::ResolveAddons(client, addons))
		FailExit();

	Log("Downloading addons...");

	std::filesystem::create_directory(szOutputDir);
	std::filesystem::current_path(szOutputDir);
	DownloadAddons(client, addons);

	return Exit("Done");
}

void ReadFile(const char* name, std::set<uintptr_t>& ids)
{
	uintptr_t entry;
	std::ifstream is(name);

	while (is >> entry)
	{
		ids.emplace(entry);
	}
}

void LoadCollections(Http::Client& client, AddonList& addons)
{
	std::set<uintptr_t> collections;
	ReadFile(szCollectionList, collections);

	for (auto collection : collections)
	{
		if (!SteamWorkshop::ResolveCollection(client, collection, addons))
			FailExit();
	}
}

void LoadStandaloneAddons(AddonList& addons)
{
	std::set<uintptr_t> addonIdList;
	ReadFile(szAddonList, addonIdList);

	for (auto id : addonIdList)
	{
		addons.try_emplace(id);
	}
}

void DownloadAddons(Http::Client& client, AddonList& addons)
{
	std::ofstream os;

	for (auto& item : addons)
	{
		auto& addon = item.second;

		if (os.open(addon.file), !os)
		{
			Log("Could not open {} Skipping...", addon.file);
			continue;
		}

		Log("Downloading {}", addon.file);

		auto res = client.Get(
			addon.url.c_str(),
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

void Log(const std::string& str)
{
	std::cout << str << std::endl;
}
