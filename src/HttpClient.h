#pragma once
#include <functional>
#include <map>
#include <string>

namespace Http
{
	typedef std::map<std::string, std::string> Params;
	typedef std::function<bool(const char* data, size_t data_length)> WriteCallback;
	static constexpr int NoError = 0;

	struct Response
	{
		int error;
		int status;
		std::string body;
	};

	class Client
	{
		private:
			void* handle;

		public:
			Client();
			~Client();

			Response Post(const char* path, const Params& params);
			Response Get(const char* path, WriteCallback writeCallback);
	};
}
