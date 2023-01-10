#pragma once
#include <map>

constexpr const char* szCollectionUrl = "https://api.steampowered.com/ISteamRemoteStorage/GetCollectionDetails/v1";
constexpr const char* szAddonUrl = "https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1";

struct AddonInfo
{
	std::string name;
	std::string file;
	uintptr_t size;
	std::string url;
	bool download;

	AddonInfo();
};

typedef std::map<uintptr_t, AddonInfo> AddonList;

struct SteamWorkshop
{
	static bool ResolveCollections(Http::Client& client, std::set<uintptr_t> collections, AddonList& addons);
	static bool ResolveAddons(Http::Client& client, AddonList& addons);
};
