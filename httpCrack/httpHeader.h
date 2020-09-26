#pragma once
#include <string>
#include <vector>

class HttpHeader {
public:
	HttpHeader();
	void check_header(const char* buffer);
	void set_crack(std::string url);
	void set_crack(std::vector<std::string> url_list);
	std::string get_url();
	bool need_crack();

private:
	std::vector<char> method;
	std::vector<char> url;
	std::vector<char> host;
	std::vector<char> cookie;
	bool need_crack_flag;
	std::vector<std::string> split(const std::string s, const std::string c);
	void set_vector_chars(std::vector<char> &s, const char* cs);
	void set_vector_chars(std::vector<char>& s, const std::string cs);
};
