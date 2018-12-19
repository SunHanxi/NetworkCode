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
	u_char icmp_type;			//��Ϣ����
	u_char icmp_code;			//����
	u_short icmp_checksum;		//У���
	u_short icmp_id;			//ͨ������Ϊ����ID
	u_short icmp_sequence;		//���к�
	u_long icmp_timestamp;		//ʱ���
}IcmpHeader;

int main(int argc, char* argv[])
{
	//����������ʱ�����Ƿ����
	if (argc == 2)
	{
		char destHost[256] = { 0 };
	}
	else
	{
		printf("�����������Ϸ���\n");
		exit(1);
	}

	//��ʼ��WinSock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("�޷���ʼ��WinSock!\n");
		exit(1);
	}

	SOCKADDR_IN Destaddr;
	HOSTENT* pointer_hostent;

	Destaddr.sin_family = AF_INET;
	if ((Destaddr.sin_addr.s_addr = inet_addr(argv[1])) == INADDR_NONE)                //IP��ַת���Ƿ�Ϸ�
	{
		pointer_hostent = gethostbyname(argv[1]);
		if (pointer_hostent != NULL)
			memcpy(&(Destaddr.sin_addr), pointer_hostent->h_addr, pointer_hostent->h_length);
		else
		{
			printf("���ܽ���Ŀ������ %s\n", argv[1]);
			ExitProcess(-1);
		}
	}

	//�������ڽ��ջ�ӦICMP����ԭʼ�׽���
	SOCKET socketRaw = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

	//����socket addr���뱾�ذ�
	struct sockaddr_in server_socket;
	server_socket.sin_family = AF_INET;
	server_socket.sin_port = 0;
	server_socket.sin_addr.S_un.S_addr = INADDR_ANY;

	//�趨��ʱʱ��
	int timeout = 1000;
	setsockopt(socketRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	//�������ڷ���udp���ݱ����׽���
	SOCKET udpSend = socket(AF_INET, SOCK_DGRAM, 0);
	//����Ŀ�ĵ��׽���
	SOCKADDR_IN destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(65511); //����һ�����������ܱ�ʹ�õĶ˿�

	//����UDP���в�������Ӧ��ICMP����
	char recvBuffer[1024] = { 0 };
	int nRet;    //���ֺ����ķ���ֵ�������жϺ����Ƿ�ɹ�ִ��
	int nTick;  //ʱ�Ӽ�ʱ
	int nTTL = 1;

	//����ICMP����ָ�룬���ڽ���Ŀ���������͵�icmp����
	IcmpHeader *pointer_ICMPHdr;
	//��������Ŀ������IP��socket addr
	SOCKADDR_IN  recvAddr;
	printf("ͨ����� 30 ��Ծ�����\n����׷�ٵ� %s ��·��\n", inet_ntoa(Destaddr.sin_addr));
	do
	{
		if (nTTL >= 2)
		{
			Sleep(1000);
		}
		//����IP�����TTL
		setsockopt(udpSend, IPPROTO_IP, IP_TTL, (char*)&nTTL, sizeof(nTTL));
		//��ȡ����ʱ��ʱ��
		nTick = GetTickCount();
		//����UDP���ݱ�
		nRet = sendto(udpSend, UDP_MESSAGE_SEND, UDP_MESSAGE_LENGTH, 0, (SOCKADDR*)&destAddr, sizeof(destAddr));
		if (nRet == SOCKET_ERROR)
		{
			printf("����UDP���ݱ�������������%d\n", WSAGetLastError());
			break;
		}

		//�ȴ���;·�ɷ��ص�icmp����
		int nLen = sizeof(recvAddr);

		nRet = recvfrom(socketRaw, recvBuffer, 1024, 0, (SOCKADDR*)&recvAddr, &nLen);
		if (nRet == SOCKET_ERROR)
		{
			printf("�������ݳ�����������%d\n", WSAGetLastError());
			continue;
		}

		//��������
		pointer_ICMPHdr = (IcmpHeader*)&recvBuffer[20]; //�ӵ�21�ֽڿ�ʼ��������Ϊǰ20�ֽ���IP����
		/*
		ICMP��������
		3  Ŀ�Ĳ��ɴ�
		11 Ŀ�곬ʱ
		*/
		if ((pointer_ICMPHdr->icmp_type != 3) && (pointer_ICMPHdr->icmp_type != 11))
		{
			printf("�������ͣ�%d\t���Ĵ���: %d\n", pointer_ICMPHdr->icmp_type, pointer_ICMPHdr->icmp_code);
		}
		else
		{
			//����·����IP
			char* routerIP = inet_ntoa(recvAddr.sin_addr);
			printf("%d  %dms   %s  \n", nTTL, GetTickCount() - nTick, routerIP);
			if (pointer_ICMPHdr->icmp_type == 3)
			{
				switch (pointer_ICMPHdr->icmp_code)
				{
				case 0:printf("Ŀ�����粻�ɴ�\n"); break;
				case 1:printf("Ŀ���������ɴ�\n"); break;
				case 6:printf("δ֪��Ŀ������\n"); break;
				case 7:printf("λ�õ�Ŀ������\n"); break;
				}
				break;
			}
		}
		if (destAddr.sin_addr.S_un.S_addr == recvAddr.sin_addr.S_un.S_addr)
		{
			printf("Ŀ���ѵ��\n");
			break;
		}

	} while (nTTL++ < 30);

	WSACleanup();
	return 0;
}