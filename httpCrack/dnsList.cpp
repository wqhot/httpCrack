#include "dnsList.h"
#include "SocketTypes.h"

unsigned long DNSList::query_ip_by_host(const char* host)
{
	mtx.lock();
	unsigned long ret = 0;
	std::map<std::string, unsigned long>::iterator it = dlist.find(host);
	if (it == dlist.end())
	{
		struct hostent *hostent = gethostbyname(host);
		if (hostent)
		{
			in_addr inad = *((in_addr*)*hostent->h_addr_list);
			ret = inad.s_addr;
			dlist[host] = ret;
		}
	}
	else {
		ret = it->second;
	}
	mtx.unlock();
	return ret;
}