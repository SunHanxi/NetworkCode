#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define UDP_MESSAGE_SEND "Hi"
#define UDP_MESSAGE_LENGTH 2
/*
 警告：Linux下必须用管理员权限运行，否则无法创建原始套接字！
 */

typedef struct icmp_hdr
{
	u_char icmp_type;			//消息类型
	u_char icmp_code;			//代码
	__u_short icmp_checksum;		//校验和
	__u_short icmp_id;			//通常设置为进程ID
	__u_short icmp_sequence;		//序列号
	u_long icmp_timestamp;		//时间戳
}IcmpHeader;

int main(int argc, char* argv[])
{
	//检查程序启动时参数是否给定
	if (argc != 2)
	{
		printf("启动参数不合法！\n");
		exit(1);
	}

	//创建目的地套接字
	struct sockaddr_in destaddr;
	struct hostent* pointer_hostent;
	//printf("IP: %s\n", inet_ntoa(Destaddr))
	destaddr.sin_port = htons(60000); //设置一个几乎不可能被使用的端口
	destaddr.sin_family = AF_INET;
	if ((destaddr.sin_addr.s_addr = inet_addr(argv[1])) == INADDR_NONE)                //IP地址转换是否合法
	{
		pointer_hostent = gethostbyname(argv[1]);
		if (pointer_hostent != NULL)
			memcpy(&(destaddr.sin_addr), pointer_hostent->h_addr_list[0], pointer_hostent->h_length);
		else
		{
			printf("不能解析目标主机 %s\n", argv[1]);
			exit(-1);
		}
	}

	struct protoent *pointer_of_protocol;  //指向协议表的指针
	//将udp与协议号进行映射
	if ((int)(pointer_of_protocol = getprotobyname("udp")) == 0)
	{
		printf("Can't find protocol number of \'udp\'!\n");
		exit(1);
	}

	//创建用于接收回应ICMP包的原始套接字
	int socketRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	//创建socket addr并与本地绑定
	struct sockaddr_in server_socket;
	server_socket.sin_family = AF_INET;
	server_socket.sin_port = 0;
	server_socket.sin_addr.s_addr = INADDR_ANY;

	//设定超时时间
	struct timeval ti; 
	ti.tv_sec=2;
    	ti.tv_usec=0;
	int mmm;
    mmm = setsockopt(socketRaw, SOL_SOCKET, SO_RCVTIMEO, &ti, sizeof(ti));	
	if (mmm == -1) {
		printf("设置超时失败\n");
	}
	
	//发送UDP序列并接收相应的ICMP报文
	char recvBuffer[1000] = { 0 };
	int nRet;    //各种函数的返回值，用来判断函数是否成功执行
	int nTick;  //时钟计时
	int nTTL = 1; //TTL
	
	//创建ICMP报文指针，用于接收目的主机回送的icmp报文
	IcmpHeader *pointer_ICMPHdr;
	//创建接收目的主机IP的socket addr
	struct sockaddr_in  recvAddr;

	memset((char*)&recvAddr, 0, sizeof(recvAddr));
	printf("通过最多 30 个跃点跟踪\n正在追踪到 %s 的路由\n", inet_ntoa(destaddr.sin_addr));
	do
	{
		if (nTTL >= 2)
		{
			sleep(1);
		}
		
		//创建用于发送udp数据报的套接字
		int udpSend = socket(AF_INET, SOCK_DGRAM, pointer_of_protocol->p_proto);

		//设置IP分组的TTL
		setsockopt(udpSend, IPPROTO_IP, IP_TTL, (char*)&nTTL, sizeof(nTTL));

		//获取发送时的时间
		time_t time_begin, time_end;
		time_begin = clock();
		//发送UDP数据报
		nRet = sendto(udpSend, UDP_MESSAGE_SEND, UDP_MESSAGE_LENGTH, 0, (struct sockaddr*)&destaddr, sizeof(destaddr));
		if (nRet == -1)
		{
			printf("发送UDP数据报出错\n");
			herror("snedto");
			break;
		}

		//等待中途路由返回的icmp报文
		int nLen = sizeof(recvAddr);

		nRet = recvfrom(socketRaw, recvBuffer, 1000, 0, (struct sockaddr*)&recvAddr, &nLen);
		if (nRet == -1)
		{
		//	printf("接收数据出错\n");
		//	herror("recvfrom");
			printf("TTL = %d, 跟踪失败，上一跳是[%s]\n",nTTL, inet_ntoa(recvAddr.sin_addr));
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
			time_end = clock();
			//解析路由器IP
			char* routerIP = inet_ntoa(recvAddr.sin_addr);
			printf("第%d跳  %ldms   %s  \n", nTTL, time_end - time_begin, routerIP);
			if (pointer_ICMPHdr->icmp_type == 3)
			{
				switch (pointer_ICMPHdr->icmp_code)
				{
				case 0:printf("目的网络不可达\n"); break;
				case 1:printf("目的主机不可达\n"); break;
				case 6:printf("未知的目的网络\n"); break;
				case 7:printf("位置的目的主机\n"); break;
				}
				//continue;
			}
		}
		if (destaddr.sin_addr.s_addr == recvAddr.sin_addr.s_addr)
		{
			printf("目标已到达！\n");
			break;
		}

	} while (nTTL++ < 30);

	close(socketRaw);
	return 0;
}
