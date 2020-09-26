#pragma once
#include <map>
#include <string>
#include <mutex>

class DNSList
{
public:
	unsigned long query_ip_by_host(const char* host);
private:
	std::mutex mtx;
	std::map<std::string, unsigned long> dlist;	// dns cache database
};

