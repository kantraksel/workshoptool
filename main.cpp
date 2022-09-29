#include <conio.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <string>

#include "version.h"
#include "httplib.h"
#include "nlohmann/json.hpp"

#if _HEADLESS
	#define GETCH()
#else
	#define GETCH() getch()
#endif

#define BASE_SUBURL "/ISteamRemoteStorage/"
const char* szBaseUrl = "https://api.steampowered.com";
const char* szCollectionUrl = BASE_SUBURL "GetCollectionDetails/v1";
const char* szAddonUrl = BASE_SUBURL "GetPublishedFileDetails/v1";
const char* szDownloadUrl = "https://steamusercontent-a.akamaihd.net";
#undef BASE_SUBURL

using PostResponse = std::function<void(std::string body)>;

struct StringTuplet
{
	std::string first;
	std::string second;
};

void Log(const char* c)
{
	std::cout << c << std::endl;
}

void Log(std::string c)
{
	std::cout << c << std::endl;
}

void ReadFile(const char* name, std::list<uintptr_t>& ids);
bool Post(httplib::Client& client, const char* path, httplib::Params& params, PostResponse response);
std::string StripDownloadUrl(std::string url);

int FailExit(const char* msg)
{
	Log(msg);
	GETCH();
	return 1;
}

int main()
{
	Log("WorkShop Tool " VERSION);

	httplib::Client client(szBaseUrl);
	client.set_ca_cert_path("./ca-bundle.crt");

	uintptr_t collection = 0;
	std::list<uintptr_t> addons;

	ReadFile("steam.wst", addons);

	//Collection is always one and first at the list
	//will change in future to support collection list
	auto i = addons.begin();
	if (i != addons.end())
	{
		collection = *i;
		++i;
		addons.pop_front();
	}
	
	if (collection != 0)
	{
		Log("Resolving collection: " + std::to_string(collection));

		httplib::Params params;
		params.emplace("collectioncount", "1");
		params.emplace("publishedfileids[0]", std::to_string(collection));

		bool result = Post(client, szCollectionUrl, params, [&](std::string body) {
			try
			{
				auto json = nlohmann::json::parse(body)["response"];
				int result = json["result"];
				int resultcount = json["resultcount"];
				if (result == resultcount == 1)
				{
					auto details = json["collectiondetails"][0]["children"];

					for (auto& element : details)
					{
						std::string sid = element["publishedfileid"];
						uintptr_t id = std::stoull(sid);
						addons.push_back(id);
						Log("Adding addon from collection: " + std::to_string(id));
					}
				}
				else Log("Response: result(" + std::to_string(result) + ") resultcount(" + std::to_string(resultcount) + ')');
			}
			catch (...)
			{
				Log("Invalid response: " + body);
			}
		});

		if (!result)
			return FailExit("Exiting...");
	}
	else Log("No collection specified");

	if (addons.empty())
		return FailExit("No addons to download. Exiting...");

	Log("Addons in queue: " + std::to_string(addons.size()));
	Log("Resolving urls...");

	httplib::Params params;
	params.emplace("itemcount", std::to_string(addons.size()));
	
	int index = 0;
	for (auto& element : addons)
	{
		params.emplace("publishedfileids[" + std::to_string(index) + ']', std::to_string(element));
		++index;
	}

	std::queue<StringTuplet> downloadQueue;
	
	bool result = Post(client, szAddonUrl, params, [&](std::string body) {
		try
		{
			auto json = nlohmann::json::parse(body)["response"];
			int result = json["result"];
			int resultcount = json["resultcount"];
			if (result == 1 && resultcount == addons.size())
			{
				auto details = json["publishedfiledetails"];

				for (auto& element : details)
				{
					result = element["result"];
					if (result != 1)
					{
						Log("Could not get details for addon " + (std::string)element["publishedfileid"]);
						continue;
					}

					auto path = std::filesystem::path((std::string)element["filename"]);
					downloadQueue.push({ element["file_url"], path.filename().string() });
				}
			}
			else Log("Response: result(" + std::to_string(result) + ") resultcount(" + std::to_string(resultcount) + ')');
		}
		catch (...)
		{
			Log("Invalid response: " + body);
		}
	});

	if (!result)
		return FailExit("Exiting...");

	if (downloadQueue.empty())
		return FailExit("No addons resolved. Exiting...");

	Log("Resolved urls: " + std::to_string(downloadQueue.size()));

	std::filesystem::create_directory("wst");
	while (!downloadQueue.empty())
	{
		auto entry = downloadQueue.front();

		std::string suburl = StripDownloadUrl(entry.first);
		if (suburl.length() == entry.first.length())
		{
			Log("Could not strip " + entry.first + " Skipping...");
			continue;
		}

		std::filesystem::path();
		std::ofstream os("wst/" + entry.second);
		if (os.fail())
		{
			Log("Could not open " + entry.second + " Skipping...");
			continue;
		}

		//dirty, but this library does not support another approach
		httplib::Client downloader(szDownloadUrl);
		downloader.set_ca_cert_path("./ca-bundle.crt");

		Log("Downloading " + entry.first);

		auto res = downloader.Get(
			suburl.c_str(), httplib::Headers(),
			[&](const char *data, size_t data_length) {
				os.write(data, data_length);
				return true;
			}
		);

		os.close();
		if (res.error() == httplib::Error::Success && res.value().status == 200) Log("Download completed!");
		else Log("Download failed!");

		downloadQueue.pop();
	}

	Log("Done");
	GETCH();
	return 0;
}

void ReadFile(const char* name, std::list<uintptr_t>& ids)
{
	uintptr_t entry;
	std::ifstream is(name);

	while (is >> entry)
	{
		ids.push_back(entry);
	}
}

bool Post(httplib::Client& client, const char* path, httplib::Params& params, PostResponse response)
{
	auto result = client.Post(path, params);
	if (result.error() != httplib::Error::Success)
	{
		Log("httplib error: " + std::to_string((int)result.error()));
		return false;
	}

	auto resultResponse = result.value();
	Log("Request status: " + std::to_string(resultResponse.status));
	if (resultResponse.status == 200)
	{
		response(resultResponse.body);
		return true;
	}

	return false;
}

std::string StripDownloadUrl(std::string url)
{
	int size = strlen(szDownloadUrl);
	if (url.length() > size)
	{
		bool match = true;
		for (int i = 0; i < size; ++i)
		{
			if (url[i] != szDownloadUrl[i])
			{
				match = false;
				break;
			}
		}

		if (match)
			url = url.substr(size);
	}

	return url;
}
