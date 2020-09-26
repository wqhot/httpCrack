#include <iostream>
#include <string>
#include <vector>
#include "Easysocket.h"
#include "httpCrack.h"
#pragma comment(lib,"ws2_32.lib")//VS need include ws2_32.dll

std::vector<std::string> hook_url_list = { "http://api.nobook.com/login/auto_check_login" };
std::vector<std::string> find_list = { "\"vip\":0" };
std::vector<std::string> replace_list = { "\"vip\":1" };


bool hook_response(std::string url, char* in, char* out)
{
	bool ret = false;
	std::cout << "HOOK URL" << url << std::endl;
	std::string in_s(in);
	for (size_t i = 0; i < hook_url_list.size(); i++)
	{	
		// Æ¥Åäµ½url
		if (strcmp(url.c_str(), hook_url_list[i].c_str()) == 0)
		{		
			// ²éÕÒÌæ»»Î»ÖÃ
			size_t pos = in_s.find(find_list[i], 0);
			std::cout << "BEFORE" << in_s << std::endl;
			if (pos > 0)
			{
				// Ìæ»»
				in_s.replace(pos, find_list[i].size(), replace_list[i]);
				ret = true;
			}
			std::cout << "AFTER" << in_s << std::endl;
			break;
		}
	}
	in_s.copy(out, in_s.size(), 0);
	*(out + in_s.size()) = '\0';
	return ret;
}

int main() 
{
	
	HttpCrack httpCrack(8080, hook_response, hook_url_list);
	httpCrack.spin();
	std::cin.get();
	return 0;

}