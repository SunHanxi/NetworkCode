#include<WinSock2.h>
#include<Windows.h>
#include<stdio.h>
#include<string.h>
#pragma comment(lib,"ws2_32.lib")
#define IP_TTL 4
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define UDP_MESSAGE_SEND "Hi"
#define UDP_MESSAGE_LENGTH 2

typedef struct icmp_hdr
{
	u_char icmp_type;			//消息类型
	u_char icmp_code;			//代码
	u_short icmp_checksum;		//校验和
	u_short icmp_id;			//通常设置为进程ID
	u_short icmp_sequence;		//序列号
	u_long icmp_timestamp;		//时间戳
}IcmpHeader;

int main(int argc, char* argv[])
{
	//检查程序启动时参数是否给定
	if (argc == 2)
	{
		char destHost[256] = { 0 };
	}
	else
	{
		printf("启动参数不合法！\n");
		exit(1);
	}

	//初始化WinSock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("无法初始化WinSock!\n");
		exit(1);
	}

	SOCKADDR_IN Destaddr;
	HOSTENT* pointer_hostent;

	Destaddr.sin_family = AF_INET;
	if ((Destaddr.sin_addr.s_addr = inet_addr(argv[1])) == INADDR_NONE)                //IP地址转换是否合法
	{
		pointer_hostent = gethostbyname(argv[1]);
		if (pointer_hostent != NULL)
			memcpy(&(Destaddr.sin_addr), pointer_hostent->h_addr, pointer_hostent->h_length);
		else
		{
			printf("不能解析目标主机 %s\n", argv[1]);
			ExitProcess(-1);
		}
	}

	//创建用于接收回应ICMP包的原始套接字
	SOCKET socketRaw = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

	//创建socket addr并与本地绑定
	struct sockaddr_in server_socket;
	server_socket.sin_family = AF_INET;
	server_socket.sin_port = 0;
	server_socket.sin_addr.S_un.S_addr = INADDR_ANY;

	//设定超时时间
	int timeout = 1000;
	setsockopt(socketRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	//创建用于发送udp数据报的套接字
	SOCKET udpSend = socket(AF_INET, SOCK_DGRAM, 0);
	//创建目的地套接字
	SOCKADDR_IN destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(65511); //设置一个几乎不可能被使用的端口

	//发送UDP序列并接收相应的ICMP报文
	char recvBuffer[1024] = { 0 };
	int nRet;    //各种函数的返回值，用来判断函数是否成功执行
	int nTick;  //时钟计时
	int nTTL = 1;

	//创建ICMP报文指针，用于接收目的主机回送的icmp报文
	IcmpHeader *pointer_ICMPHdr;
	//创建接收目的主机IP的socket addr
	SOCKADDR_IN  recvAddr;
	printf("通过最多 30 个跃点跟踪\n正在追踪到 %s 的路由\n", inet_ntoa(Destaddr.sin_addr));
	do
	{
		if (nTTL >= 2)
		{
			Sleep(1000);
		}
		//设置IP分组的TTL
		setsockopt(udpSend, IPPROTO_IP, IP_TTL, (char*)&nTTL, sizeof(nTTL));
		//获取发送时的时间
		nTick = GetTickCount();
		//发送UDP数据报
		nRet = sendto(udpSend, UDP_MESSAGE_SEND, UDP_MESSAGE_LENGTH, 0, (SOCKADDR*)&destAddr, sizeof(destAddr));
		if (nRet == SOCKET_ERROR)
		{
			printf("发送UDP数据报出错，错误码是%d\n", WSAGetLastError());
			break;
		}

		//等待中途路由返回的icmp报文
		int nLen = sizeof(recvAddr);

		nRet = recvfrom(socketRaw, recvBuffer, 1024, 0, (SOCKADDR*)&recvAddr, &nLen);
		if (nRet == SOCKET_ERROR)
		{
			printf("接收数据出错，错误码是%d\n", WSAGetLastError());
			continue;
		}

		//解析报文
		pointer_ICMPHdr = (IcmpHeader*)&recvBuffer[20]; //从第21字节开始解析，因为前20字节是IP报文
		/*
		ICMP报文类型
		3  目的不可达
		11 目标超时
		*/
		if ((pointer_ICMPHdr->icmp_type != 3) && (pointer_ICMPHdr->icmp_type != 11))
		{
			printf("报文类型：%d\t报文代码: %d\n", pointer_ICMPHdr->icmp_type, pointer_ICMPHdr->icmp_code);
		}
		else
		{
			//解析路由器IP
			char* routerIP = inet_ntoa(recvAddr.sin_addr);
			printf("%d  %dms   %s  \n", nTTL, GetTickCount() - nTick, routerIP);
			if (pointer_ICMPHdr->icmp_type == 3)
			{
				switch (pointer_ICMPHdr->icmp_code)
				{
				case 0:printf("目的网络不可达\n"); break;
				case 1:printf("目的主机不可达\n"); break;
				case 6:printf("未知的目的网络\n"); break;
				case 7:printf("位置的目的主机\n"); break;
				}
				break;
			}
		}
		if (destAddr.sin_addr.S_un.S_addr == recvAddr.sin_addr.S_un.S_addr)
		{
			printf("目标已到达！\n");
			break;
		}

	} while (nTTL++ < 30);

	WSACleanup();
	return 0;
}