/*
Windows x86平台,基于Visual Studio2017编译
VS2017编译时需要使用_CRT_SECURE_NO_DEPRECATE来规避scanf错误
*/
#if defined _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#include<winsock2.h>
#include<Windows.h>
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
typedef struct protoent PROTOENT;
typedef struct sockaddr_in SOCKADDR_IN;
typedef unsigned int SOCKET;
typedef struct sockaddr SOCKADDR;

#else
#define closesocket close
#define h_addr h_addr_list[0] // for backward compatibility
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif // !linux

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define PROTOPORT 9999    //默认端口号

int main(int argc, char* argv[])
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(WINSOCK_VERSION, &wsaData);
#endif
	PROTOENT* pointer_of_protocol;
	SOCKADDR_IN client_addr;
	SOCKADDR_IN server_addr;
	SOCKET socket_descriptor;
	int port;
	int addr_len = sizeof(server_addr);
	char buffer[1000] = { 0 };
	////初始化服务端socket地址
	//memset((char*)&server_addr, 0, sizeof(server_addr));

	//设置协议族
	server_addr.sin_family = AF_INET;

	/*
	s_addr的定义如下
	#define s_addr  S_un.S_addr  //can be used for most tcp & ip code
	windows支持上述完整定义，linux只支持s_addr这种方式
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
	if (((int)(pointer_of_protocol = getprotobyname("udp"))) == 0)
	{
		printf("Can't find protocol number of \'udp\'!\n");
		exit(1);
	}

	//创建socket
	socket_descriptor = socket(AF_INET, SOCK_DGRAM, pointer_of_protocol->p_proto);
	if (socket_descriptor < 0)
	{
		printf("Can't create socket!\n");
		exit(1);
	}

	//绑定本地地址
	int bind_ret = bind(socket_descriptor, (SOCKADDR_IN *)&server_addr, addr_len);
	if (bind_ret < 0)
	{
		printf("Can't bind local address!\n");
		exit(1);
	}

	//开始接收客户端发送来的数据
	printf("Server is started!\nServer is waiting for data.\n");
	while (1)
	{
		//printf("Buffer length is %d\n", strlen(buffer));
		int receive_data;
		receive_data = recvfrom(socket_descriptor, buffer, 1000, 0, (SOCKADDR*)&client_addr, &addr_len);
		if (receive_data < 0)
		{
			printf("Length of Data is:%d\n", receive_data);
			printf("Recieve Data Fail!\n");
			exit(1);
		}
		printf("buff is :\n%s\n", buffer);
		char buffer[1000] = { 0 };
	}

	closesocket(socket_descriptor);

#ifdef _WIN32
	WSACleanup();
#endif // WIN32
	return 0;
}
