#include "httpHeader.h"

HttpHeader::HttpHeader()
{
	need_crack_flag = false;
}

void HttpHeader::check_header(const char* buffer)
{
	std::vector<std::string> p = split(buffer, "\r\n");
	if (p.size() == 0)
	{
		return;
	}
	const std::string get_head = "GET";
	const std::string post_head = "POST";

	// 如果需要修改http请求包头，就在这里修改
	if (p[0].compare(0, get_head.size(), get_head) == 0) {  //GET方式
		std::vector<std::string> f_l = split(p[0], " ");
		if (f_l.size() != 3)
		{
			return;
		}
		set_vector_chars(method, get_head);
		set_vector_chars(url, f_l[1]);
	}
	else if (p[0].compare(0, post_head.size(), post_head) == 0) {  //POST方式
		std::vector<std::string> f_l = split(p[0], " ");
		if (f_l.size() != 3)
		{
			return;
		}
		set_vector_chars(method, post_head);
		set_vector_chars(url, f_l[1]);
	}
}

void HttpHeader::set_crack(std::string url)
{
	need_crack_flag = true;
	if (url.size() != HttpHeader::url.size())
	{
		need_crack_flag = false;
		return;
	}
	for (size_t i = 0; i != HttpHeader::url.size(); i++)
	{
		if (url[i] != HttpHeader::url[i])
		{
			need_crack_flag = false;
			break;
		}
	}
}

void HttpHeader::set_crack(std::vector<std::string> url_list)
{
	need_crack_flag = false;
	for (auto url : url_list)
	{		
		if (url.size() != HttpHeader::url.size())
		{
			need_crack_flag = false;
			continue;
		}
		for (size_t i = 0; i != HttpHeader::url.size(); i++)
		{
			if (url[i] != HttpHeader::url[i])
			{
				need_crack_flag = false;
				break;
			}
		}
		need_crack_flag = true;
	}
}

std::string HttpHeader::get_url()
{
	std::string ret;
	for (int i = 0; i < url.size(); ++i) {
		ret += url[i];
	}
	ret += '\0';
	return ret;
}

bool HttpHeader::need_crack()
{
	return need_crack_flag;
}


void HttpHeader::set_vector_chars(std::vector<char>& s, const char* cs)
{
	s.clear();
	const char *p = cs;
	while (*p != 0)
	{
		s.push_back(*p);
		p++;
	}
}

void HttpHeader::set_vector_chars(std::vector<char>& s, const std::string cs)
{
	s.clear();
	for (auto p : cs)
	{
		s.push_back(p);
	}
}

std::vector<std::string> HttpHeader::split(const std::string s, const std::string c)
{
	std::vector<std::string> v;
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
	return v;
}

