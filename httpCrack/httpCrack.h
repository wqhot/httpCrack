#pragma once
#include "Easysocket.h"
#include "dnsList.h"
#include <thread>
#include <condition_variable>
#include <string>
#include <functional>
#include <vector>

struct HTTP_PARAM
{
	SocketLib::DataSocket client_socket;
	SocketLib::DataSocket server_socket;
	bool need_crack = false;
	std::string url = "";
};

struct MULTI_WAIT
{
	std::mutex mtx;
	std::condition_variable cond;
	int count = 0;
	void add()
	{
		std::unique_lock<std::mutex> lock(mtx);
		++count;
		cond.notify_all();
		lock.unlock();
	}
	void wait(int total_count)
	{
		while (count < total_count)
		{
			std::unique_lock<std::mutex> lock(mtx);
			cond.wait(lock);
			lock.unlock();
		}
	}
};

class HttpCrack
{
public:
	typedef std::function<bool(std::string, char* in, char* out)> hook_func;
	HttpCrack(int port, hook_func func, std::vector<std::string> hook_url_list);
	void spin();  // 主循环
	bool connect_server(HTTP_PARAM& http_param, char* recv_buf, int len); // 连接服务器
	bool init_host(HTTP_PARAM& http_param, char* host_name, unsigned short port);  // 获取域名IP
	int recv_request(HTTP_PARAM http_param, char* buf, int bufSize); // 接收request
	int pre_response(HTTP_PARAM& http_param);
	int send_data(SocketLib::DataSocket s, const char* buf, int bufSize);
	int send_data(SocketLib::sock s, const char* buf, int bufSize);
	int	exchange_data(HTTP_PARAM http_param);
	bool send_web_request(HTTP_PARAM http_param, char* send_buf, char* recv_buf, int data_len);
	hook_func m_hook_func;

private:
	SocketLib::ListeningSocket lsock;
	DNSList dnslist;
	std::vector<std::string> m_hook_url_list;
	static int circle(HttpCrack* httpCrack, HTTP_PARAM http_param);
	static int exc_thread(HttpCrack* httpCrack, HTTP_PARAM* http_param_, MULTI_WAIT *multi_wait_);
	

};

