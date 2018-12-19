/*
Windows平台,基于Visual Studio2017编译
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



#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#pragma comment(lib,"ws2_32.lib")

#define PROTOPORT 9999  //设定发送数据的端口号

extern int ErrorNo;   //全局变量？ 错误代码

/*
设定目标主机的域名，已在hosts文件中设定
*/
char default_hostname[] = "localhost";

//主程序开始
int main(int argc, char* argv[])
{
	struct hostent *pointer_of_host;  //指向主机表条目的指针
	struct protoent *pointer_of_protocol;  //指向协议表的指针
	struct sockaddr_in socket_address;    //指向ip地址的指针
	int socket_descriptor;				//socket描述符
	int port;							//端口号
	char *host;							//主机名指针
	//int number_of_read;					//读的字符的数量
	//char buffer[1000];					//缓冲区大小

#ifdef WIN32
	//windows的socket协议必须启动的东西
	WSADATA wsaDATA;
	WSAStartup(0x0101, &wsaDATA);
#endif // WIN32

	memset((char*)&socket_address, 0, sizeof(socket_address));
	socket_address.sin_family = AF_INET;  //指定协议族

	//若程序启动时指定了端口号，则将其写入port，否则使用默认port:9999
	if (argc > 2)
	{
		port = atoi(argv[2]);
	}
	else
	{
		port = PROTOPORT;
	}
	if (port > 0)
	{
		//端口号转为网络地址
		socket_address.sin_port = htons((u_short)port);

	}
	else
	{
		printf("Error Port:%s\n", argv[2]);
		exit(1);
	}

	//若程序启动时制定了主机名，则将其设定，否则使用默认的主机，即局域网的主机192.168.118.128
	if (argc > 1)
	{
		host = argv[1];
	}
	else
	{
		host = default_hostname;
	}

	//将主机名转换为ip地址
	pointer_of_host = gethostbyname(host);
	if (((char*)pointer_of_host) == NULL)
	{
		printf("Error Host:%s", argv[1]);
	}

	memcpy(&socket_address.sin_addr, pointer_of_host->h_addr, pointer_of_host->h_length);

	//将tcp与协议号进行映射
	if ((int)(pointer_of_protocol = getprotobyname("tcp")) == 0)
	{
		printf("Can't find protocol number of \'tcp\'!\n");
		exit(1);
	}

	//创建socket
	socket_descriptor = socket(PF_INET, SOCK_STREAM, pointer_of_protocol->p_proto);
	if (socket_descriptor < 0)
	{
		printf("Can't create socket!\n");
		exit(1);
	}

	//连接至特定server进行debug测试
	if ((connect(socket_descriptor, (struct sockaddr*)&socket_address, sizeof(socket_address))) < 0)
	{
		printf("Can't connect to server");
		exit(1);
	}

	//向服务器发送数据
	char buffer[1000] = { 0 };
	printf("Pleast insert the message sent to server:\n");
	int length = scanf("%s", buffer);
	if (length > 1000)
	{
		printf("Data is Too Long!\n");
		exit(1);
	}
	//sprintf(buffer,"Hello!\n");
	printf("Length of Buffer: %d\n", strlen(buffer));
	int ret = send(socket_descriptor, buffer, strlen(buffer), 0);
	if (ret < 0)
	{
		printf("Send Information Failed!\n");
		exit(1);
	}
	else
	{
		printf("Send successfully!\n");
	}

	//关闭socket连接
	closesocket(socket_descriptor);
#ifdef _WIN32
	WSACleanup();
#endif // WIN32
	return 0;
}
