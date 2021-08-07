#pragma once
#include <curl/curl.h>
#include <string>
#include <map>
#include <mutex>

namespace http
{
	struct data_blob {
		const char* buffer;
		size_t size;
	};

	enum class method { GET = CURLOPT_HTTPGET, POST = CURLOPT_POST };

	typedef std::map<std::string, std::string> dict;

	static size_t write_callback(void* ptr, size_t size, size_t nmemb, std::string* buffer);
	static size_t read_callback(char* dest, size_t size, size_t nmemb, void* userp);

	static bool init = false;
	static int instances = 0;
	static std::mutex mutex;

	class parameters
	{
	private:
		std::map<std::string, std::string> _parameters;
	public:
		parameters(std::map<std::string, std::string> parameters = {});

		std::string json(std::map<std::string, std::string> parameters = {});
	};

	class response
	{
	private:
		int _code;
		std::string _data;
		std::string _headers;
		std::string _cookies;

	public:
		response(int code = 0, std::string data = {}, std::string headers = {}, std::string cookies = {});

		http::dict headers();
		std::string text();
		int code();
	};

	class HttpClient
	{
	private:
		CURL* curl;

	public:
		HttpClient();
		~HttpClient();

		std::string url_encode(std::string data);

		http::response REQUEST(http::method method, std::string url, std::string data = std::string(), http::dict headers = { {} });
	};
}