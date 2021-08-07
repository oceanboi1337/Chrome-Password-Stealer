#include "http.hpp"

#include <iostream>
#include <sstream>
#include <vector>

http::HttpClient::HttpClient()
{
	http::mutex.lock();

	if (!init)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		init = true;
	}

	http::mutex.unlock();

	this->curl = curl_easy_init();
	curl_easy_setopt(this->curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(this->curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, http::write_callback);
	curl_easy_setopt(this->curl, CURLOPT_READFUNCTION, http::read_callback);
	curl_easy_setopt(this->curl, CURLOPT_FAILONERROR, 1L);

	http::instances++;

#ifdef _DEBUG
	curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 1L);
#endif
}

http::HttpClient::~HttpClient()
{
	http::mutex.lock();

	if (http::instances <= 0)
	{
		std::cout << http::instances << std::endl;
		curl_global_cleanup();
	}
	else
	{
		http::instances--;
	}
	curl_easy_cleanup(this->curl);

	http::mutex.unlock();
}

std::string http::HttpClient::url_encode(std::string data)
{
	return curl_easy_escape(this->curl, data.data(), data.size());
}

http::response http::HttpClient::REQUEST(http::method method, std::string url, std::string data, http::dict headers)
{
	int response_code;
	std::string data_buffer;
	std::string headers_buffer;
	std::string cookies_buffer;

	curl_easy_setopt(this->curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, &data_buffer);
	curl_easy_setopt(this->curl, CURLOPT_HEADERDATA, &headers_buffer);
	curl_easy_setopt(this->curl, CURLOPT_COOKIELIST, &cookies_buffer);
	curl_easy_setopt(this->curl, static_cast<CURLoption>(method), 1L);

	struct curl_slist* curl_headers = NULL;
	for (const auto& header : headers)
	{
		std::stringstream ss;
		ss << header.first << ": " << header.second;
		curl_headers = curl_slist_append(curl_headers, ss.str().c_str());
	}
	curl_easy_setopt(this->curl, CURLOPT_HTTPHEADER, curl_headers);

	char* buffer = (char*)malloc(data.size());

	if (method == http::method::POST)
	{
		if (buffer != NULL)
		{
			std::memset(buffer, '\0', data.size());
			std::memcpy(buffer, data.data(), data.size());
		}

		curl_easy_setopt(this->curl, CURLOPT_POSTFIELDSIZE, data.size());
		curl_easy_setopt(this->curl, CURLOPT_POSTFIELDS, data.data());
	}

	std::free(buffer);
	curl_easy_perform(this->curl);

	curl_easy_getinfo(this->curl, CURLINFO_RESPONSE_CODE, &response_code);

	return http::response(response_code, data_buffer, headers_buffer, cookies_buffer);
}

http::parameters::parameters(std::map<std::string, std::string> parameters)
{
	this->_parameters = parameters;
}

std::string http::parameters::json(std::map<std::string, std::string> parameters)
{
	if (parameters.size() == 0)
	{
		parameters = this->_parameters;
	}

	std::stringstream ss;
	ss << '{';

	for (const auto& param : parameters)
	{
		ss << '"' << param.first << '"' << ": " << '"' << param.second << '"' << ',';
	}

	ss.seekp(-1, std::ios_base::end);
	ss << '}';

	return ss.str();
}

http::response::response(int code, std::string data, std::string headers, std::string cookies)
{
	this->_code = code;
	this->_data = data;
	this->_headers = headers;
	this->_cookies = cookies;
}

http::dict http::response::headers()
{
	return http::dict();
}

std::string http::response::text()
{
	return this->_data;
}

int http::response::code()
{
	return this->_code;
}

size_t http::write_callback(void* ptr, size_t size, size_t nmemb, std::string* buffer)
{
	buffer->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

size_t http::read_callback(char* dest, size_t size, size_t nmemb, void* userp)
{
	struct data_blob* data = (struct data_blob*)(userp);
	size_t buffer_size = size * nmemb;

	if (data->size)
	{
		size_t copy_size = data->size;
		if (copy_size > buffer_size)
		{
			copy_size = buffer_size;
		}
		std::memcpy(dest, data->buffer, copy_size);

		data->buffer += copy_size;
		data->size -= copy_size;
		return copy_size;
	}
	return 0;
}
