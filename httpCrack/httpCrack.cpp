#include "httpCrack.h"
#include <iostream>

#include "httpHeader.h"

#define MAX_LENGTH 65536
#define MAX_HOST_NAME 512
#define HEADLEN 7 // http://

std::string GetIpFromULong(unsigned long uIp)
{
	in_addr addr;

	memcpy(&addr, &uIp, sizeof(uIp));

	std::string strIp = inet_ntoa(addr);

	return strIp;
}


HttpCrack::HttpCrack(int port, hook_func func, std::vector<std::string> hook_url_list)
{
	m_hook_url_list = hook_url_list;
	m_hook_func = func;
	lsock.Listen(port); // port 8080	
}

void HttpCrack::spin()
{
	SocketLib::DataSocket dsock;
	std::cout << "http crack start" << std::endl;
	while (1)
	{
		dsock = lsock.Accept();
		HTTP_PARAM http_param;
		http_param.client_socket = dsock;
		std::thread th = std::thread(circle, this, http_param);
		th.detach();
	}
}

int HttpCrack::circle(HttpCrack *httpCrack, HTTP_PARAM http_param)
{
	char *recv_buf = new char[MAX_LENGTH]();
	char *send_buf = new char[MAX_LENGTH]();
	int retval = httpCrack->recv_request(http_param, recv_buf, MAX_LENGTH);
	//std::cout << "recv_buf " << recv_buf << std::endl;
	if (retval <= 0)
	{
		http_param.client_socket.Close();
		return -1;
	}
	if (strncmp("CONNECT ", recv_buf, 8) == 0) // CONNECT 代理
	{
		// cout << "Request connection" << endl;
		if (!httpCrack->connect_server(http_param, recv_buf, retval))
		{ 
			http_param.client_socket.Close();
			return -1;
		}
		if (httpCrack->pre_response(http_param) < 0)
		{ 
			http_param.client_socket.Close();
			http_param.server_socket.Close();
			return -1;
		}
		httpCrack->exchange_data(http_param);	// 连接已经建立，只需交换数据
	}
	else	// 直接转发
	{
		httpCrack->send_web_request(http_param, send_buf, recv_buf, retval);
	}
	delete[] recv_buf;
	delete[] send_buf;
	return 0;
}

bool HttpCrack::send_web_request(HTTP_PARAM http_param, char* send_buf, char* recv_buf, int data_len)
{
	char host_name[MAX_HOST_NAME] = { 0 };
	int port = 80;
	HttpHeader httpHeader;
	httpHeader.check_header(recv_buf);
	httpHeader.set_crack(m_hook_url_list);

	char* line = strstr(recv_buf, "\r\n");	// 一定有 因为request head 已经判定了\r\n

	char* url_begin = strchr(recv_buf, ' ');
	if (!url_begin) { return 0; }
	char* path_begin = strchr(url_begin + 1 + HEADLEN, '/');
	if (!path_begin) { return 0; }


	char* port_begin = (char*)(memchr(url_begin + 1 + HEADLEN, ':', path_begin - url_begin - HEADLEN));
	char* host_end = path_begin;
	if (port_begin)	// 有端口
	{
		host_end = port_begin;
		char buf_port[64] = { 0 };
		memcpy(buf_port, port_begin + 1, path_begin - port_begin - 1);
		port = atoi(buf_port);
	}
	memcpy(host_name, url_begin + 1 + HEADLEN, host_end - url_begin - 1 - HEADLEN);

	//Trace
	char line_buf[0x1000] = { 0 };
	int leng = line - recv_buf;
	if (leng < sizeof(line_buf))
	{
		memcpy(line_buf, recv_buf, leng);
	}
	else {
		const static int lenc = 50;
		memcpy(line_buf, recv_buf, lenc);
		strcpy(line_buf + lenc, " ... ");
		memcpy(line_buf + lenc + 5, line - 16, 16);
	}

	if (!init_host(http_param, host_name, port))
	{
		return 0;
	}

	memcpy(send_buf, recv_buf, url_begin - recv_buf + 1); // 填写method
	memcpy(send_buf + (url_begin - recv_buf) + 1, path_begin, recv_buf + data_len - path_begin);	// 填写剩余内容

	char* HTTPTag = strstr(send_buf + (url_begin - recv_buf) + 1, " HTTP/1.1\r\n");
	if (HTTPTag) { HTTPTag[8] = '0'; }	// 强制使用http/1.0 防止Keep-Alive

	size_t total_len = url_begin + 1 + data_len - path_begin;

	if (send_data(http_param.server_socket, send_buf, total_len) <= 0) 
	{ 
		return 0; 
	}

	http_param.need_crack = httpHeader.need_crack();
	http_param.url = httpHeader.get_url();
	exchange_data(http_param);
	return true;
}

int HttpCrack::exc_thread(HttpCrack *httpCrack, HTTP_PARAM *http_param_, MULTI_WAIT *multi_wait_)
{
	HTTP_PARAM *http_param = http_param_;
	MULTI_WAIT *multi_wait = multi_wait_;
	bool need_crack = http_param->need_crack;
	int ret = 0;
	char* buf = new char[MAX_LENGTH]();
	char* buf_n = new char[MAX_LENGTH]();
	// 不断把s1的数据转移到s2;
	while (1)
	{
		try
		{
			ret = http_param->client_socket.Receive(buf, MAX_LENGTH);
		}
		catch (...)
		{
			delete[] buf;
			delete[] buf_n;
			multi_wait->add();
			return ret;
		}
		if (ret <= 0) 
		{ 
			delete[] buf;
			delete[] buf_n;
			multi_wait->add();
			return ret; 
		}
		if (need_crack)
		{
			httpCrack->m_hook_func(http_param->url, buf, buf_n);
			// crack_response(buf, buf_n);
			std::cout << "=====================" << std::endl;
		}
		else
		{
			memcpy(buf_n, buf, ret);
		}	

		ret = httpCrack->send_data(http_param->server_socket, buf_n, ret);
		//std::cout << "send data to " << GetIpFromULong(http_param->server_socket.GetRemoteAddress()) << ":" << http_param->server_socket.GetRemotePort() << std::endl;
		if (ret <= 0) 
		{ 
			delete[] buf;
			delete[] buf_n;
			multi_wait->add();
			return ret; 
		}
	}
	delete[] buf;
	delete[] buf_n;
	multi_wait->add();
	return 0;
}

int	HttpCrack::exchange_data(HTTP_PARAM http_param)
{
	HTTP_PARAM http_param_1 = http_param;
	HTTP_PARAM http_param_2 = { http_param.server_socket, http_param.client_socket, http_param.need_crack, http_param.url};
	//std::cout << "before swap client_socket = " << http_param_1.get_client_socket().GetSock() << "->"<< GetIpFromULong(http_param_1.get_client_socket().GetRemoteAddress()) << ":" << http_param_1.get_client_socket().GetRemotePort() << std::endl;
	//std::cout << "before swap server_socket = " << http_param_1.get_server_socket().GetSock() << "->" << GetIpFromULong(http_param_1.get_server_socket().GetRemoteAddress()) << ":" << http_param_1.get_server_socket().GetRemotePort() << std::endl;
	//std::cout << "after swap client_socket = " << http_param_2.get_client_socket().GetSock() << "->" << GetIpFromULong(http_param_2.get_client_socket().GetRemoteAddress()) << ":" << http_param_2.get_client_socket().GetRemotePort() << std::endl;
	//std::cout << "after swap server_socket = " << http_param_2.get_server_socket().GetSock() << "->" << GetIpFromULong(http_param_2.get_server_socket().GetRemoteAddress()) << ":" << http_param_2.get_server_socket().GetRemotePort() << std::endl;
	MULTI_WAIT multi_wait;
	std::thread th1 = std::thread(exc_thread, this, &http_param_1, &multi_wait);
	std::thread th2 = std::thread(exc_thread, this, &http_param_2, &multi_wait);
	
	multi_wait.wait(1);
	try
	{
		http_param.server_socket.Close();
	}
	catch (...)
	{
		std::cout << "catch client_socket close" << std::endl;
	}
	
	try
	{
		http_param.client_socket.Close();
	}
	catch (...)
	{
		std::cout << "catch server_socket close" << std::endl;
	}
	multi_wait.wait(2);
	th2.join();
	th1.join();
	return 0;
}

int HttpCrack::send_data(SocketLib::sock s, const char* buf, int bufSize)
{
	int pos = 0;
	while (pos < bufSize)
	{
		int ret = 0;
		ret = send(s, buf + pos, bufSize - pos, 0);
		if (ret > 0) {
			pos += ret;
		}
		else {
			return ret;
		}
	}
	return pos;
}

int HttpCrack::send_data(SocketLib::DataSocket s, const char* buf, int bufSize)
{
	int pos = 0;
	while (pos < bufSize)
	{
		int ret = 0;
		try
		{
			ret = s.Send(buf + pos, bufSize - pos);
		}
		catch (...)
		{
			return ret;
		}
		
		if (ret > 0) {
			pos += ret;
		}
		else {
			return ret;
		}
	}
	return pos;
}

int HttpCrack::pre_response(HTTP_PARAM& http_param)
{
	const char response[] = "HTTP/1.1 200 Connection established\r\n"
		"Proxy-agent: HTTP Proxy Lite /0.2\r\n\r\n";   // 这里是代理程序的名称，看你的是什么代理软件"
	int ret = send_data(http_param.client_socket, response, sizeof(response) - 1);
	if (ret <= 0) { return -2; }
	return 0;
}

bool HttpCrack::connect_server(HTTP_PARAM &http_param, char* recv_buf, int len)
{
	// 从	recvBuf 解析 host 和 port
	char str_host[MAX_HOST_NAME] = { 0 };
	char str_port[8] = { 0 };	// < 65535
	unsigned short port = 80;
	char* sp = (char*)(memchr(recv_buf + 8, ' ', len - 8));
	if (!sp) 
	{ 
		return false; 
	}
	char* pt = (char*)(memchr(recv_buf + 8, ':', sp - recv_buf - 8));
	if (pt)
	{
		int l = sp - pt - 1;
		if (l >= 8) 
		{ 
			return false; 
		}
		memcpy(str_port, pt + 1, l);
		port = atoi(str_port);
		memcpy(str_host, recv_buf + 8, pt - recv_buf - 8);
	}
	else {
		memcpy(str_host, recv_buf + 8, sp - recv_buf - 8);
	}

	return init_host(http_param, str_host, port);
}

bool HttpCrack::init_host(HTTP_PARAM& http_param, char* host_name, unsigned short port)
{
	try
	{
		http_param.server_socket.Connect(
			dnslist.query_ip_by_host(host_name),
			port
		);
	}
	catch (...)
	{
		http_param.server_socket.Close();
		return false;
	}

	std::cout << "CONNECT " << host_name << ":" << port << std::endl;

	return true;
}

int HttpCrack::recv_request(HTTP_PARAM http_param, char* buf, int bufSize)
{
	int len = 0;
	char* prf = buf;
	while (len < bufSize)
	{
		int ret = 0;
		try
		{
			ret = http_param.client_socket.Receive(buf, bufSize - len);
		}
		catch (...)
		{
			return ret;
		}
		if (ret > 0) 
		{
			len += ret;
		}
		else 
		{
			return ret;
		}
		// Find End Tag: \r\n\r\n
		if (len > 4)
		{
			if (strstr(prf, "\r\n\r\n"))
			{
				break;
			}
			else {
				prf = buf + len - 4;
			}
		}
	}
	return len;
}