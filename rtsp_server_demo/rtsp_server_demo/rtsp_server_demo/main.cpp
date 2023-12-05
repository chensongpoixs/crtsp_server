/***********************************************************************************************
created: 		2023-12-06

author:			chensong

purpose:		config
 
************************************************************************************************/


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")
#include <stdint.h>
#include <windows.h>
#pragma warning( disable : 4996 )

 
static const uint16_t  DEFAULT_RTSP_PORT = 8554;


static const uint16_t   DEFAULT_SERVER_RTP_PORT = 55532;
static const uint16_t   DEFAULT_SERVER_RTCP_PORT = 55533;


static void init_network()
{
	//初始化 DLL
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

}


static int create_tcp_socket()
{
	int sockfd;
	int on = 1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		return -1;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

	return sockfd;
}

static int bind_socket_addr(int sockfd, const char *ip, uint16_t port)
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
	{
		return -1;
	}

	return 0;
}

static int accept_client(int sockfd, char *client_ip, uint16_t * client_port)
{
	int clientfd;
	socklen_t len = 0;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	len = sizeof(addr);

	clientfd = accept(sockfd, (struct sockaddr*)&addr, &len);
	if (clientfd < 0)
		return -1;

	strcpy(client_ip, inet_ntoa(addr.sin_addr));
	*client_port = ntohs(addr.sin_port);

	return clientfd;
}


static int handle_cmd_options  (char* result, int cseq)
{
	sprintf(result, "RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
		"\r\n",
		cseq);

	return 0;
}

static int handle_cmd_describe  (char* result, int cseq, char* url)
{
	char sdp[500];
	char localIp[100];

	sscanf(url, "rtsp://%[^:]:", localIp);

	sprintf(sdp, "v=0\r\n"
		"o=- 9%ld 1 IN IP4 %s\r\n"
		"t=0 0\r\n"
		"a=control:*\r\n"
		"m=video 0 RTP/AVP 96\r\n"
		"a=rtpmap:96 H264/90000\r\n"
		"a=control:track0\r\n",
		time(NULL), localIp);

	sprintf(result, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
		"Content-Base: %s\r\n"
		"Content-type: application/sdp\r\n"
		"Content-length: %zu\r\n\r\n"
		"%s",
		cseq,
		url,
		strlen(sdp),
		sdp);

	return 0;
}

static int handle_cmd_setup(char* result, int cseq, int clientRtpPort)
{
	sprintf(result, "RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
		"Session: 66334873\r\n"
		"\r\n",
		cseq,
		clientRtpPort,
		clientRtpPort + 1,
		DEFAULT_SERVER_RTP_PORT,
		DEFAULT_SERVER_RTCP_PORT);

	return 0;
}

static int handle_cmd_play(char* result, int cseq)
{
	sprintf(result, "RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Range: npt=0.000-\r\n"
		"Session: 66334873; timeout=10\r\n\r\n",
		cseq);

	return 0;
}



static int do_client(int fd, char *client_ip, uint16_t   client_port)
{

	char method[40] = {0};
	char url[100] = {0};
	char version[40] = {0};
	int32_t CSeq = 0;

	uint16_t client_rtp_port;
	uint16_t client_rtcp_port;

	char * rbuf = (char *)::malloc(10000);
	char * sbuf = (char *)::malloc(10000);


	while (true)
	{
		int32_t recv_len = 0;
		recv_len = ::recv(fd, rbuf, 2000, 0);
		if (recv_len <= 0)
		{
			break;
		}
		rbuf[recv_len] = '\0';

		std::string recvstr = rbuf;
		printf("==========================================================\n");
		printf("%s [rbuf = %s]\n", __FUNCTION__, rbuf);

		const char * sep = "\n";
		char *line = strtok(rbuf, sep);
		while (line)
		{
			if (strstr(line, "OPTIONS") ||
				strstr(line, "DESCRIBE") ||
				strstr(line, "SETUP") ||
				strstr(line, "PLAY")
				)
			{
				if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
				{
					printf("[%s][%d][ERROR]\n", __FUNCTION__, __LINE__);
				}
				else if (strstr(line, "CSeq"))
				{
					if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1)
					{
						printf("[%s][%d][ERROR]\n", __FUNCTION__, __LINE__);
					}
				}
				else if (!strncmp(line, "Transport:", strlen("Transport:")))
				{
					// Transport: RTP/AVP/UDP;unicast;client_port=13358-13359
					// Transport: RTP/AVP;unicast;client_port=13358-13359
					if (sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n", &client_rtp_port, &client_rtcp_port) != 2)
					{
						printf("[%s][%d][ERROR]Transport\n", __FUNCTION__, __LINE__);
					}
				}
				line = strtok(NULL, sep);
			}
			if (!strcmp(method, "OPTIONS"))
			{
				if (handle_cmd_options(sbuf, CSeq))
				{
					printf("handle option failed !!!\n");
					break;
				}

			}
			else if (!strcmp(method, "DESCRIBE"))
			{
				if (handle_cmd_describe(sbuf, CSeq, url))
				{
					printf("handle describe failed !!!\n");
					break;
				}
			}
			else if (!strcmp(method, "SETUP"))
			{
				if (handle_cmd_setup(sbuf, CSeq, client_rtp_port))
				{
					printf("handle setup failed !!!\n");
					break;
				}
			}
			else if (!strcmp(method, "PLAY"))
			{
				if (handle_cmd_play(sbuf, CSeq))
				{
					printf("handle play failed !!!\n");
					break;
				}
			}
			else {
				printf("未定义的method = %s \n", method);
				break;
			}
			printf("-------------------------------------------\n");
			printf("%s sbuf = %s\n", __FUNCTION__, sbuf);

			::send(fd, sbuf, strlen(sbuf), 0);
			//开始播放 发送RTP包
			if (!strcmp(method, "PLAY"))
			{
				printf("start play !!!\n");
				printf("client ip = %s\n", client_ip);
				printf("client_port = %u\n", client_port);
				while (true)
				{
					Sleep(40);
				}
				break;
			}
			memset(method, 0, sizeof(method) / sizeof(char));
			memset(url, 0, sizeof(url) / sizeof(char));
			CSeq = 0;
		}
	}
	
	::closesocket(fd);
	free(rbuf);
	free(sbuf);

	return -1;
}
 

 

	int main(int argc, char *argv[])
	{
		init_network();



		int serversocketfd = create_tcp_socket();

		if (serversocketfd < 0)
		{
			::WSACleanup();
			printf("failed to create tcp socket !!!\n");
			return -1;
		}

		if (bind_socket_addr(serversocketfd, "0.0.0.0", DEFAULT_RTSP_PORT) < 0)
		{
			printf("bind addr failed !!!\n");
			return -1;
		}

		if (::listen(serversocketfd, 10) < 0)
		{
			printf("listen failed !!!\n");
			return -1;
		}

		printf("%s rtsp://127.0.0.1:%d\n", __FILE__, DEFAULT_RTSP_PORT);


		while (true)
		{
			//int client_fd;
			char client_ip[40] = { 0 };
			uint16_t client_port;

			int client_fd = accept_client(serversocketfd, client_ip, &client_port);
			if (client_fd < 0)
			{
				printf("accept client failed !!!\n");
				return -1;
			}

			printf("accept client:client ip = %s, client port = %u\n", client_ip, client_port);

			do_client(client_fd, client_ip, client_port);
		}

		::closesocket(serversocketfd);
		return 0;
	}