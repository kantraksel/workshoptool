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
};

typedef std::map<uintptr_t, AddonInfo> AddonList;

struct SteamWorkshop
{
	static bool ResolveCollection(Http::Client& client, uintptr_t collectionId, AddonList& addons);
	static bool ResolveAddons(Http::Client& client, AddonList& addons);
};
