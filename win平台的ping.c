#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include<Windows.h>
#include <stdio.h>
#include <stdlib.h>
#define ICMP_ECHO 8	          // ������Դ���8 ping����  
#define ICMP_ECHOREPLY 0      // ����Ӧ�����0
#define ICMP_MIN 12           // ICMP����ͷ��12�ֽ�
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

//����IPͷ��20�ֽ�
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
	unsigned int sourceIP;			  // ԴIP��ַ
	unsigned int destIP;			  // Ŀ��IP��ַ
}IpHeader;

/* ICMP header 12�ֽ� */
typedef  struct _ihdr {
	BYTE  i_type;				//��ʶICMP���ĵ�����
	BYTE  i_code;              //��ʶ��ӦICMP���ĵĴ��롣���������ֶ�һ��ͬ��ʶ��ICMP���ĵ���ϸ���͡�
	USHORT  i_cksum;			//У���
	USHORT  i_id;				//��ʶ��
	USHORT  i_seq;				//���
	/* This is not the std header,  but we  reserve space for time */
	ULONG  timestamp;			//ʱ���
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


	if ((sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == INVALID_SOCKET)
	{
		fprintf(stderr, "WSAStartup failed: %d\n", GetLastError());
		ExitProcess(STATUS_FAILED);
	}

	memset(&dest, 0, sizeof(dest));
	//VS2017��Ҫ�ر�SDL���
	//��Ŀ�����ԡ�C/C++�������SDL���
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
		Sleep(1000);
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
			printf("Ŀ�겻�ɴ\n");
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