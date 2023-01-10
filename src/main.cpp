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

#if ENABLEGETCH
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
	Log("Workshop Tool " VERSION);

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
	Log("Loading collections from {}", szCollectionList);
	std::set<uintptr_t> collections;
	ReadFile(szCollectionList, collections);
	Log("Loaded collections: {}", collections.size());

	if (collections.empty())
		return;

	Log("Downloading collection info...");
	if (!SteamWorkshop::ResolveCollections(client, collections, addons))
		FailExit();
}

void LoadStandaloneAddons(AddonList& addons)
{
	Log("Loading addons from {}", szAddonList);
	std::set<uintptr_t> addonIdList;
	ReadFile(szAddonList, addonIdList);

	if (addonIdList.empty())
	{
		Log("Loaded addons: 0");
		return;
	}

	int addonsAdded = 0;
	int addonsOmitted = 0;

	for (auto id : addonIdList)
	{
		auto result = addons.try_emplace(id);
		if (result.second) ++addonsAdded;
		else ++addonsOmitted;
	}

	Log("Queued addons: {} added, {} duplicates omitted", addonsAdded, addonsOmitted);
}

std::string ParseSize(uintptr_t size);

void DownloadAddons(Http::Client& client, AddonList& addons)
{
	std::ofstream os;

	for (auto& item : addons)
	{
		auto& addon = item.second;

		if (!addon.download)
			continue;

		if (os.open(addon.file, std::ios::binary), !os)
		{
			Log("Could not open {} Skipping...", addon.file);
			continue;
		}

		Log("Downloading {} (size: {} file: {})", addon.name, ParseSize(addon.size), addon.file);

		auto res = client.Get(
			addon.url.c_str(),
			[&](const char* data, size_t data_length) {
				os.write(data, data_length);
				return (bool)os;
			}
		);

		os.close();
		if (res.error == Http::NoError && res.status == 200) Log("Download completed");
		else Log("Download failed!");
	}
}

std::string ParseSize(uintptr_t size)
{
	constexpr uintptr_t kB = 1024;
	constexpr uintptr_t MB = kB * 1024;
	constexpr uintptr_t GB = MB * 1024;

	float fsize;
	const char* unit = nullptr;

	if (size > GB)
	{
		fsize = (float)size / GB;
		unit = "GB";
	}
	else if (size > MB)
	{
		fsize = (float)size / MB;
		unit = "MB";
	}
	else if (size > kB)
	{
		fsize = (float)size / kB;
		unit = "kB";
	}
	else
	{
		fsize = size;
		unit = "B";
	}

	return std::format("{:.2f}{}", fsize, unit);
}

void Log(const std::string& str)
{
	std::cout << str << std::endl;
}
