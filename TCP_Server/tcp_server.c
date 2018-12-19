/*
Windows x86平台,基于Visual Studio2017编译
VS2017编译时需要使用_CRT_SECURE_NO_DEPRECATE来规避scanf错误
*/
#if defined _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#include<Windows.h>
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")

/*
Linux平台预处理文件
基于gcc version 7.3.0 & Ubuntu18.04 LTS
*/
#elif defined(__linux__) 
#define u_short __u_short
#define closesocket close
#define h_addr h_addr_list[0] // for backward compatibility
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#else
#define closesocket close
#define h_addr h_addr_list[0]
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define PROTOPORT 9999    //默认端口号
#define QUEUE_LENGTH 6    //默认队列长度为6

int visitors = 0;   //客户端连接数

int main(int argc, char* argv[])
{
	//struct hostent *pointer_to_host;
	struct protoent *pointer_to_protocol;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	int socket_descriptor_1, socket_descriptor_2;
	int port;
	int addr_len;
	char buffer[1000] = {0};

#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(0x0101, &wsaData);
#endif

	//初始化服务端socket地址
	memset((char*)&server_addr, 0, sizeof(server_addr));

	//设置协议族
	server_addr.sin_family = AF_INET;
	
	/*
	s_addr的定义如下
	#define s_addr  S_un.S_addr  //can be used for most tcp & ip code
	INADDR_ANY 为允许任意ip地址连接至服务端
	*/
	server_addr.sin_addr.s_addr = INADDR_ANY;

	//检查程序启动时是否指定了端口号
	if (argc > 1) 
		port = atoi(argv[1]);
	else 
		port = PROTOPORT;
	if (port > 0)
		server_addr.sin_port = htons((u_short)port);
	else
	{
		printf("Port Error!\n");
		exit(1);
	}

	if (((int)(pointer_to_protocol = getprotobyname("tcp"))) == 0)
	{
		printf("Can't find protocol number of \'tcp\'!\n");
		exit(1);
	}

	//创建socket
	socket_descriptor_1 = socket(PF_INET, SOCK_STREAM, pointer_to_protocol->p_proto);
	if (socket_descriptor_1 < 0)
	{
		printf("Can't create socket!\n");
		exit(1);
	}
	
	//将本地地址绑定至socket
	int bind_ret;
	bind_ret = bind(socket_descriptor_1, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (bind_ret < 0)
	{
		printf("Can't bind local address!\n");
		exit(1);
	}

	//启动监听队列
	int listen_ret = listen(socket_descriptor_1, QUEUE_LENGTH);
	if (listen_ret < 0)
	{
		printf("Can't listen!\n");
		exit(1);
	}

	//主循环，用于循环处理连接
	while (1)
	{
		addr_len = sizeof(client_addr);
		//与客户连接时创建新的socket，用于数据交换
		socket_descriptor_2 = accept(socket_descriptor_1, (struct sockaddr*)&client_addr, &addr_len);
		if (socket_descriptor_2 < 0)
		{
			printf("Can't accept the client!\n");
			exit(1);
		}
		visitors++;
		printf("\nThis server has been contacted %d time%s\n",visitors, visitors == 1 ? "." : "s.");
		
		//接收客户端发送的数据		
		int n = recv(socket_descriptor_2, buffer, sizeof(buffer), 0);
		while (n > 0) 
		{
			write(1, buffer, n);
			n = recv(socket_descriptor_2, buffer, sizeof(buffer), 0);
		}
		//printf("%s\n", buffer);
		//关闭socket连接
		closesocket(socket_descriptor_2);
	}

	
#ifdef _WIN32
	WSACleanup();
#endif // WIN32
	return 0;
}
