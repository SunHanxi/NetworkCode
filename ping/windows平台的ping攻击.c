#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include<Windows.h>
#include <stdio.h>
#include <stdlib.h>
#define ICMP_ECHO 8	          // 请求回显代号8 ping请求  
#define ICMP_ECHOREPLY 0      // 回显应答代号0
#define ICMP_MIN 12           // ICMP报文头，12字节
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAX_DATA_LENGTH 991   //1024-32-1

//定义IP头，20字节
typedef struct iphdr {
	unsigned char h_len : 4;          // length of the header
	unsigned char version : 4;        // Version of IP
	unsigned char tos;                // Type of service
	unsigned short total_len;         // total length of the packet
	unsigned short ident;             // unique identifier
	unsigned short frag_and_flags;    // flags
	unsigned char  ttl;
	unsigned char proto;              // protocol (TCP, UDP etc)
	unsigned short checksum;          // IP checksum
	unsigned int sourceIP;			  // 源IP地址
	unsigned int destIP;			  // 目的IP地址
}IpHeader;

/* ICMP header 12字节 */
typedef  struct _ihdr {
	BYTE  i_type;				//标识ICMP报文的类型
	BYTE  i_code;              //标识对应ICMP报文的代码。它与类型字段一起共同标识了ICMP报文的详细类型。
	USHORT  i_cksum;			//校验和
	USHORT  i_id;				//标识符
	USHORT  i_seq;				//序号
	/* This is not the std header,  but we  reserve space for time */
	ULONG  timestamp;			//时间戳
	char i_data[MAX_DATA_LENGTH];
}IcmpHeader;

#define  STATUS_FAILED 0xFFFF
#define  MAX_PACKET 1024		
#define  xmalloc(s) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))
#define  xfree(p)   HeapFree(GetProcessHeap(),0,(p))
#define DEFAULT_HOST "localhost"

USHORT checksum(USHORT *, int);
void fill_icmp_head(char *);
void decode_resp(char *, int, struct sockaddr_in *);

void Usage(char *progname)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "%s <host>\n", progname);
	ExitProcess(STATUS_FAILED);
}

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET sockRaw;
	struct sockaddr_in dest, from;
	struct hostent *hp;
	int bread, datasize;
	int fromlen = sizeof(from);
	char *dest_ip;
	char *icmp_data;
	char *recvbuf;
	unsigned int addr = 0;
	USHORT seq_no = 0;
	if (WSAStartup(WINSOCK_VERSION, &wsaData) != 0)
	{
		fprintf(stderr, "WSAStartup failed: %d\n", GetLastError());
		ExitProcess(STATUS_FAILED);
	}
	/*if (argc < 2)
	{
		Usage(argv[0]);
	}*/

	if ((sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == INVALID_SOCKET)
	{
		fprintf(stderr, "WSAStartup failed: %d\n", GetLastError());
		ExitProcess(STATUS_FAILED);
	}

	memset(&dest, 0, sizeof(dest));
	//VS2017需要关闭SDL检查
	//项目→属性→C/C++→常规→SDL检查
	if (argc > 1)
	{
		hp = gethostbyname(argv[1]);
		if (hp != NULL)
		{
			memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length);
			dest.sin_family = AF_INET;
			dest_ip = inet_ntoa(dest.sin_addr);
		}
		else
		{
			fprintf(stderr, "Unable to resolve %s\n", argv[1]);
			ExitProcess(STATUS_FAILED);
		}
	}
	else
	{
		hp = gethostbyname(DEFAULT_HOST);
		if (hp != NULL)
		{
			memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length);
			dest.sin_family = AF_INET;
			dest_ip = inet_ntoa(dest.sin_addr);
		}
		else
		{
			fprintf(stderr, "Unable to resolve %s\n", DEFAULT_HOST);
			ExitProcess(STATUS_FAILED);
		}
	}
	

	datasize = sizeof(IcmpHeader);	    // 12byte 
	icmp_data = xmalloc(MAX_PACKET);	// 1024byte
	recvbuf = xmalloc(MAX_PACKET);	    // 1024byte

	if (!icmp_data) 
	{
		fprintf(stderr, "HeapAlloc failed %d\n", GetLastError());
		ExitProcess(STATUS_FAILED);
	}

	memset(icmp_data, 0, MAX_PACKET);
	fill_icmp_head(icmp_data);

	//填充icmp包的攻击数据
	for (int i = 0; i < MAX_DATA_LENGTH; i++)
	{
		((IcmpHeader*)icmp_data)->i_data[i] = 'a';
	}
	//printf("%s\n", ((IcmpHeader*)icmp_data)->i_data);

	while (1) 
	{
		int bwrote;
		((IcmpHeader*)icmp_data)->i_cksum = 0;
		((IcmpHeader*)icmp_data)->timestamp = GetTickCount();
		((IcmpHeader*)icmp_data)->i_seq = seq_no++;
		((IcmpHeader*)icmp_data)->i_cksum = checksum((USHORT*)icmp_data, sizeof(IcmpHeader));
		//((IcmpHeader*)icmp_data)->i_data = buffer;
		bwrote = sendto(sockRaw, icmp_data, datasize, 0, (struct sockaddr*)&dest, sizeof(dest));
		if (bwrote == SOCKET_ERROR) 
		{
			fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
			ExitProcess(STATUS_FAILED);
		}
		if (bwrote < datasize) 
		{
			fprintf(stdout, "Wrote %d bytes\n", bwrote);
		}
		bread = recvfrom(sockRaw, recvbuf, MAX_PACKET, 0, (struct sockaddr*)&from, &fromlen);
		if (bread == SOCKET_ERROR) 
		{
			if (WSAGetLastError() == WSAETIMEDOUT)
			{
				printf("timed out\n");
				continue;
			}
			fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
			perror("revffrom failed.");
			ExitProcess(STATUS_FAILED);
		}
		decode_resp(recvbuf, bread, &from);
		//Sleep(1);
	}
	closesocket(sockRaw);
	xfree(icmp_data);
	xfree(recvbuf);
	WSACleanup();    /* clean up ws2_32.dll */
	return 0;
}

void fill_icmp_head(char * icmp_data) 
{

	IcmpHeader *icmp_hdr;

	icmp_hdr = (IcmpHeader*)icmp_data;
	icmp_hdr->i_type = ICMP_ECHO;
	icmp_hdr->i_code = 0;
	icmp_hdr->i_cksum = 0;
	icmp_hdr->i_id = (USHORT)GetCurrentProcessId();
	icmp_hdr->i_seq = 0;
}

void decode_resp(char *buf, int bytes, struct sockaddr_in *from)
{
	IpHeader *iphdr;
	IcmpHeader *icmphdr;
	unsigned short iphdrlen;

	iphdr = (IpHeader *)buf;
	iphdrlen = iphdr->h_len * 4; // number of 32-bit words *4 = bytes

	if (bytes < iphdrlen + ICMP_MIN) {
		printf("Too few bytes from %s\n", inet_ntoa(from->sin_addr));
	}
	icmphdr = (IcmpHeader*)(buf + iphdrlen);

	if (icmphdr->i_type != ICMP_ECHOREPLY) 
	{
		fprintf(stderr, "non-echo type %d recvd\n", icmphdr->i_type);
		if (icmphdr->i_type == 3)
		{
			printf("目标不可达！\n");
		}
		return;
	}
	if (icmphdr->i_id != (USHORT)GetCurrentProcessId())
	{
		fprintf(stderr, "someone else's packet!\n");
		return;
	}
	printf("%d bytes from %s:", bytes, inet_ntoa(from->sin_addr));
	printf(" icmp_seq = %d. ", icmphdr->i_seq);
	printf(" time: %d ms ", GetTickCount() - icmphdr->timestamp);
	printf("\n");
}
USHORT checksum(USHORT *buffer, int size)
{
	unsigned long cksum = 0;

	while (size > 1) 
	{
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}

	if (size) 
	{
		cksum += *(UCHAR*)buffer;
	}

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}
