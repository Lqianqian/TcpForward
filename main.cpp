// Email: me@lyshark.com
// PowerBy: ZXPortMap,LyShark

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAXBUFSIZE 8192
#define MAXSTACK 1024*2

#include <iostream>
#include <winsock2.h> 

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ����ת��
namespace ForwardForwarding
{
	struct SOCKINFO
	{
		SOCKET ClientSock;
		SOCKET ServerSock;
	};

	struct ADDRESS
	{
		char szIP[32];
		WORD wPort;
		SOCKET s;
	};

	// ����һ��ģ���� STACK
	template<typename T>
	class STACK
	{
	private:
		// ˽�г�Ա����
		int top;            // ջ��Ԫ�ص�����
		T Data[MAXSTACK];   // ���ڴ洢ջԪ�ص�����

	public:
		// ���г�Ա����

		// ���캯��
		STACK()
		{
			top = -1;       // ��ջ��������ʼ��Ϊ -1����ʾջΪ��
		}

		// �ж�ջ�Ƿ�Ϊ�յĺ���
		bool IsEmpty()
		{
			return top < 0;  // ���ջ������С�� 0����ջΪ��
		}

		// �ж�ջ�Ƿ������ĺ���
		bool IsFull()
		{
			return top >= MAXSTACK;  // ���ջ���������ڵ��� MAXSTACK����ջ����
		}

		// ��Ԫ����ջ�ĺ���
		bool Push(T data)
		{
			if (IsFull())
				return false;   // ���ջ����������ջʧ��
			top++;              // ջ�������� 1
			Data[top] = data;   // ��Ԫ�ش���ջ��
			return true;        // ��ջ�ɹ�
		}

		// ��Ԫ�س�ջ�ĺ���
		T Pop()
		{
			return Data[top--]; // ��ջ��Ԫ�ص���������ջ�������� 1
		}
	};

	// ����һ��ģ���� TransferParam����������ģ�����Ͳ��� X �� Y
	template<typename X, typename Y>
	class TransferParam
	{
	public:
		// ���г�Ա����
		X GlobalData;           // ȫ�����ݣ�����Ϊ X
		STACK<Y> LocalData;     // �ֲ����ݣ�����Ϊ STACK<Y>

	public:
		// ���г�Ա����

		// Ĭ�Ϲ��캯��
		TransferParam();

		// ������ѹ��ֲ�ջ�ĺ���
		bool Push(Y data);

		// �Ӿֲ�ջ�е������ݵĺ���
		Y Pop();
	};

	// TransferParam ģ����Ĺ��캯��
	template<typename X, typename Y>
	TransferParam<X, Y>::TransferParam()
	{
		memset(this, 0, sizeof(TransferParam)); // ���ö����ڴ�ռ�����
	}

	// ��ֲ�ջ��ѹ�����ݵĺ���
	template<typename X, typename Y>
	bool TransferParam<X, Y>::Push(Y data)
	{
		return LocalData.Push(data); // ���� STACK<Y> ���͵� LocalData ����� Push ����
	}

	// �Ӿֲ�ջ�е������ݵĺ���
	template<typename X, typename Y>
	Y TransferParam<X, Y>::Pop()
	{
		return LocalData.Pop(data); // ���� STACK<Y> ���͵� LocalData ����� Pop ����
	}

	// ��DataBuf�е�DataLen���ֽڷ����׽���s����
	int DataSend(SOCKET s, char *DataBuf, int DataLen)
	{
		int nBytesLeft = DataLen;    // ʣ��δ���͵��ֽ���
		int nBytesSent = 0;          // �ѷ��͵��ֽ���
		int ret;                     // send �����ķ���ֵ
		int iMode = 0;              // ������ģʽ
		ioctlsocket(s, FIONBIO, (u_long FAR*) &iMode);  // �����׽���Ϊ������ģʽ
		while (nBytesLeft > 0)
		{
			ret = send(s, DataBuf + nBytesSent, nBytesLeft, 0);  // ��������

			printf("[ ���� ] %d bytes \n", strlen(DataBuf));    // ����ѷ������ݵĳ���
			if (ret <= 0)
			{
				break;
			}
			nBytesSent += ret;
			nBytesLeft -= ret;
		}
		return nBytesSent;  // �����ѷ��͵��ֽ���
	}

	// �̺߳���������SOCKET�н�������ת��
	DWORD WINAPI TransmitData(LPVOID lParam)
	{
		SOCKINFO socks = *((SOCKINFO*)lParam);
		SOCKET ClientSock = socks.ClientSock;
		SOCKET ServerSock = socks.ServerSock;
		char RecvBuf[MAXBUFSIZE] = { 0 };
		fd_set Fd_Read;
		int ret, nRecv;

		while (1)
		{
			FD_ZERO(&Fd_Read);
			FD_SET(ClientSock, &Fd_Read);
			FD_SET(ServerSock, &Fd_Read);
			ret = select(0, &Fd_Read, NULL, NULL, NULL);
			if (ret <= 0)
			{
				goto error;
			}

			if (FD_ISSET(ClientSock, &Fd_Read))
			{
				nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
				printf("[ �հ� ] %d bytes \n", strlen(RecvBuf));

				if (nRecv <= 0)
				{
					goto error;
				}

				ret = DataSend(ServerSock, RecvBuf, nRecv);
				if (ret == 0 || ret != nRecv)
				{
					goto error;
				}
			}

			if (FD_ISSET(ServerSock, &Fd_Read))
			{
				nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
				printf("[ �հ� ] %d bytes \n", strlen(RecvBuf));
				if (nRecv <= 0)
				{
					goto error;
				}

				ret = DataSend(ClientSock, RecvBuf, nRecv);
				if (ret == 0 || ret != nRecv)
				{
					goto error;
				}
			}
		}
	error:
		closesocket(ClientSock);
		closesocket(ServerSock);
		return 0;
	}

	// ��װ��������ָ����IP�Ͷ˿�
	SOCKET ConnectHost(char* szIP, WORD wPort)
	{
		SOCKET sockid;
		if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			return 0;
		}

		struct sockaddr_in srv_addr;
		srv_addr.sin_family = AF_INET;
		srv_addr.sin_addr.S_un.S_addr = inet_addr(szIP);
		srv_addr.sin_port = htons(wPort);
		if (connect(sockid, (struct sockaddr*)&srv_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			return 0;
		}

		return sockid;
	}

	// ��dwIP�ϰ�wPort�˿�
	SOCKET CreateSocket(DWORD dwIP, WORD wPort)
	{
		SOCKET sockid;
		if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			return 0;
		}

		struct sockaddr_in srv_addr = { 0 };
		srv_addr.sin_family = AF_INET;
		srv_addr.sin_addr.S_un.S_addr = dwIP;
		srv_addr.sin_port = htons(wPort);
		if (bind(sockid, (struct sockaddr*)&srv_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			closesocket(sockid);
			return 0;
		}

		listen(sockid, 3);
		return sockid;
	}

	// ִ�ж˿�ӳ�亯��
	DWORD WINAPI PortTransfer(LPVOID lParam)
	{
		TransferParam<ADDRESS, SOCKET> *ConfigInfo = (TransferParam<ADDRESS, SOCKET>*)lParam;
		SOCKET ClientSock, ServerSock;

		// ��ջ��ÿͻ����׽���
		ClientSock = ConfigInfo->LocalData.Pop();

		// �����ӵ�Ŀ�������ķ���
		ServerSock = ConnectHost(ConfigInfo->GlobalData.szIP, ConfigInfo->GlobalData.wPort);
		if (ServerSock <= 0)
		{
			closesocket(ClientSock);
			return 0;
		}

		SOCKINFO socks;
		socks.ClientSock = ClientSock;       // �ͻ����׽���
		socks.ServerSock = ServerSock;       // Ŀ������������׽���
		return TransmitData((LPVOID)&socks); // ��������ת��״̬
	}

	// ת������
	BOOL Forwarding(WORD ListenPort, PCHAR szIP, WORD wPort)
	{
		// ִ��ӳ�亯��
		HANDLE hThread;
		SOCKET AcceptSocket;
		TransferParam<ADDRESS, SOCKET> ConfigInfo;

		// ����ַ
		_snprintf(ConfigInfo.GlobalData.szIP, 32, "%s", szIP);
		ConfigInfo.GlobalData.wPort = wPort;

		// ��������˿� ��ӳ��˿�
		SOCKET localsockid = CreateSocket(INADDR_ANY, ListenPort);
		if (localsockid <= 0)
		{
			closesocket(localsockid);
			return FALSE;
		}
		while (1)
		{
			AcceptSocket = accept(localsockid, NULL, NULL);
			if (AcceptSocket == INVALID_SOCKET)
			{
				closesocket(localsockid);
				return FALSE;
			}

			// �����ܵ��Ŀͻ������׽���ת���µ��߳��ﴦ��
			ConfigInfo.LocalData.Push(AcceptSocket);
			hThread = CreateThread(0, 0, PortTransfer, (LPVOID)&ConfigInfo, 0, 0);
			CloseHandle(hThread);
		}
	}
}

// ����˿�ת��
namespace ReverseForwarding
{
	struct SOCKINFO
	{
		SOCKET ClientSock;
		SOCKET ServerSock;
	};

	struct ADDRESS
	{
		char szIP[32];
		WORD wPort;
		SOCKET s;
	};

	template<typename T>
	class STACK
	{
	private:
		int top;
		T Data[MAXSTACK];
	public:
		STACK()
		{
			top = -1;
		}
		bool IsEmpty()
		{
			return top < 0;
		}
		bool IsFull()
		{
			return top >= MAXSTACK;
		}
		bool Push(T data)
		{
			if (IsFull())
			{
				return false;
			}
			top++;
			Data[top] = data;
			return true;
		}
		T Pop()
		{
			return Data[top--];
		}
	};

	template<typename X, typename Y>
	class TransferParam
	{
	public:
		X GlobalData;
		STACK<Y> LocalData;
	public:
		TransferParam();
		bool Push(Y data);
		Y Pop();
	};

	template<typename X, typename Y>
	TransferParam<X, Y>::TransferParam()
	{
		memset(this, 0, sizeof(TransferParam));
	}

	template<typename X, typename Y>
	bool TransferParam<X, Y>::Push(Y data)
	{
		return LocalData.Push(data);
	}

	template<typename X, typename Y>
	Y TransferParam<X, Y>::Pop()
	{
		return LocalData.Pop(data);
	}

	int DataSend(SOCKET s, char *DataBuf, int DataLen)
	{
		int nBytesLeft = DataLen;
		int nBytesSent = 0;
		int ret;
		int iMode = 0;
		ioctlsocket(s, FIONBIO, (u_long FAR*) &iMode);
		while (nBytesLeft > 0)
		{
			ret = send(s, DataBuf + nBytesSent, nBytesLeft, 0);
			printf("[ ���� ] %d bytes \n", strlen(DataBuf));
			if (ret <= 0)
			{
				break;
			}
			nBytesSent += ret;
			nBytesLeft -= ret;
		}
		return nBytesSent;
	}

	DWORD WINAPI TransmitData(LPVOID lParam)
	{
		SOCKINFO socks = *((SOCKINFO*)lParam);
		SOCKET ClientSock = socks.ClientSock;
		SOCKET ServerSock = socks.ServerSock;
		char RecvBuf[MAXBUFSIZE] = { 0 };
		fd_set Fd_Read;
		int ret, nRecv;
		while (1)
		{
			FD_ZERO(&Fd_Read);
			FD_SET(ClientSock, &Fd_Read);
			FD_SET(ServerSock, &Fd_Read);
			ret = select(0, &Fd_Read, NULL, NULL, NULL);
			if (ret <= 0)
			{
				goto error;
			}

			if (FD_ISSET(ClientSock, &Fd_Read))
			{
				nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
				printf("[ �հ� ] %d bytes \n", strlen(RecvBuf));
				if (nRecv <= 0)
				{
					goto error;
				}
				ret = DataSend(ServerSock, RecvBuf, nRecv);
				if (ret == 0 || ret != nRecv)
				{
					goto error;
				}
			}

			if (FD_ISSET(ServerSock, &Fd_Read))
			{
				nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
				printf("[ �հ� ] %d bytes \n", strlen(RecvBuf));
				if (nRecv <= 0)
				{
					goto error;
				}

				ret = DataSend(ClientSock, RecvBuf, nRecv);
				if (ret == 0 || ret != nRecv)
				{
					goto error;
				}
			}
		}
	error:
		closesocket(ClientSock);
		closesocket(ServerSock);
		return 0;
	}

	SOCKET ConnectHost(DWORD dwIP, WORD wPort)
	{
		SOCKET sockid;
		if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			return 0;
		}

		struct sockaddr_in srv_addr;
		srv_addr.sin_family = AF_INET;
		srv_addr.sin_addr.S_un.S_addr = dwIP;
		srv_addr.sin_port = htons(wPort);
		if (connect(sockid, (struct sockaddr*)&srv_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			goto error;
		}

		return sockid;
	error:
		closesocket(sockid);
		return 0;
	}

	SOCKET ConnectHost(char *szIP, WORD wPort)
	{
		return ConnectHost(inet_addr(szIP), wPort);
	}

	SOCKET CreateSocket(DWORD dwIP, WORD wPort)
	{
		SOCKET sockid;
		if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			return 0;
		}

		struct sockaddr_in srv_addr = { 0 };
		srv_addr.sin_family = AF_INET;
		srv_addr.sin_addr.S_un.S_addr = dwIP;
		srv_addr.sin_port = htons(wPort);
		if (bind(sockid, (struct sockaddr*)&srv_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			goto error;
		}

		listen(sockid, 3);
		return sockid;
	error:
		closesocket(sockid);
		return 0;
	}

	// ����һ����ʱ���׽���,ָ��wPort��ô�������ʱ�˿�
	SOCKET CreateTmpSocket(WORD *wPort)
	{
		struct sockaddr_in srv_addr = { 0 };
		int addrlen = sizeof(struct sockaddr_in);
		SOCKET s = CreateSocket(INADDR_ANY, 0);
		if (s <= 0)
		{
			goto error;
		}
		if (getsockname(s, (struct sockaddr*)&srv_addr, &addrlen) == SOCKET_ERROR)
		{
			goto error;
		}
		*wPort = ntohs(srv_addr.sin_port);
		return s;
	error:
		closesocket(s);
		return 0;
	}

	// �˿�ת���ͻ���
	DWORD WINAPI PortTransferClient(LPVOID lParam)
	{
		TransferParam<ADDRESS, WORD> *ConfigInfo = (TransferParam<ADDRESS, WORD> *)lParam;
		SOCKET CtrlSocket = ConfigInfo->GlobalData.s;
		DWORD dwCtrlIP;
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		if (getpeername(CtrlSocket, (SOCKADDR *)&clientaddr, &addrlen) == SOCKET_ERROR)
		{
			return 0;
		}

		// �������PortTransfer_3ģʽ�ļ������IP
		dwCtrlIP = clientaddr.sin_addr.S_un.S_addr;
		// wPort = ntohs(clientaddr.sin_port);

		SOCKET ClientSocket, ServerSocket;
		SOCKINFO socks;

		// ���������µ�����
		ClientSocket = ConnectHost(dwCtrlIP, ConfigInfo->LocalData.Pop());
		if (ClientSocket <= 0)
		{
			return 0;
		}

		// ���ӵ�Ŀ�������ķ���
		ServerSocket = ConnectHost(ConfigInfo->GlobalData.szIP, ConfigInfo->GlobalData.wPort);
		if (ServerSocket <= 0)
		{
			closesocket(ClientSocket);
			return 0;
		}

		// ת�����ݰ�
		socks.ClientSock = ClientSocket;    // ������������׽���
		socks.ServerSock = ServerSocket;    // Ŀ������������׽���
		return TransmitData((LPVOID)&socks);
	}

	BOOL PortTransferClient(char *szCtrlIP, WORD wCtrlPort, char *szIP, WORD wPort)
	{
		int nRecv;
		WORD ReqPort;
		HANDLE hThread;
		DWORD dwThreadId;
		TransferParam<ADDRESS, WORD> ConfigInfo;
		_snprintf(ConfigInfo.GlobalData.szIP, 32, "%s", szIP);
		ConfigInfo.GlobalData.wPort = wPort;

		// ��PortTransfer_3ģʽ�������ڹ������ļ�����������ƹܵ�����
		SOCKET CtrlSocket = ConnectHost(szCtrlIP, wCtrlPort);
		if (CtrlSocket <= 0)
		{
			goto error;
		}

		ConfigInfo.GlobalData.s = CtrlSocket;
		while (1)
		{
			// �������Թ�����������������Ϊһ��WORD����ʾ�������������������˿�
			nRecv = recv(CtrlSocket, (char*)&ReqPort, sizeof(ReqPort), 0);
			if (nRecv <= 0)
			{
				goto error;
			}

			// ������Ϣ�Ľṹ
			ConfigInfo.LocalData.Push(ReqPort);
			hThread = CreateThread(NULL, 0, PortTransferClient, (LPVOID)&ConfigInfo, NULL, &dwThreadId);
			if (hThread)
			{
				CloseHandle(hThread);
			}
			else
			{
				Sleep(1000);
			}
		}
	error:
		closesocket(CtrlSocket);
		return false;
	}

	// �˿�ת�������
	DWORD WINAPI PortTransferServer(LPVOID lParam)
	{
		SOCKINFO socks;
		SOCKET ClientSocket, ServerSocket, CtrlSocket, tmpSocket;
		TransferParam<SOCKET, SOCKET> *ConfigInfo = (TransferParam<SOCKET, SOCKET>*)lParam;
		CtrlSocket = ConfigInfo->GlobalData;
		ClientSocket = ConfigInfo->LocalData.Pop();
		WORD wPort;

		// ������ʱ�˿�
		tmpSocket = CreateTmpSocket(&wPort);
		if (tmpSocket <= 0 || wPort <= 0)
		{
			closesocket(ClientSocket);
			return 0;
		}

		// ֪ͨ�����û������µ����ӵ��մ�������ʱ�˿�
		if (send(CtrlSocket, (char*)&wPort, sizeof(wPort), 0) == SOCKET_ERROR)
		{
			closesocket(ClientSocket);
			closesocket(CtrlSocket);
			return 0;
		}

		ServerSocket = accept(tmpSocket, NULL, NULL);
		if (ServerSocket == INVALID_SOCKET)
		{
			closesocket(ClientSocket);
			return 0;
		}

		// ��ʼת��
		socks.ClientSock = ClientSocket;
		socks.ServerSock = ServerSocket;
		return TransmitData((LPVOID)&socks);
	}

	// �����������˿�
	BOOL PortTransferServer(WORD wCtrlPort, WORD wServerPort)
	{
		HANDLE hThread;
		BOOL bOptVal = TRUE;
		int bOptLen = sizeof(BOOL);
		TransferParam<SOCKET, SOCKET> ConfigInfo;
		SOCKET ctrlsockid, serversockid, CtrlSocket, AcceptSocket;

		// �����׽���
		ctrlsockid = CreateSocket(INADDR_ANY, wCtrlPort);
		if (ctrlsockid <= 0)
		{
			goto error2;
		}

		// �����׽���
		serversockid = CreateSocket(INADDR_ANY, wServerPort);
		if (serversockid <= 0)
		{
			goto error1;
		}

		// �������ԣ������û�����PortTransfer_2ģʽ�������ƹܵ����ӵ�����
		CtrlSocket = accept(ctrlsockid, NULL, NULL);
		if (CtrlSocket == INVALID_SOCKET)
		{
			goto error0;
		}

		if (setsockopt(CtrlSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&bOptVal, bOptLen) == SOCKET_ERROR)
		{
			goto error0;
		}

		// �������û����������Ӻ���൱�˿�ӳ��ɹ���
		// ׼��������շ�������״̬��������������߳���ͨ�����ƹܵ�֪ͨ�����û������µ����ӽ�������ת��
		ConfigInfo.GlobalData = CtrlSocket;
		while (1)
		{
			// �����¿ͻ�
			AcceptSocket = accept(serversockid, NULL, NULL);
			if (AcceptSocket == INVALID_SOCKET)
			{
				Sleep(1000);
				continue;
			}

			// �ѽ��ܵ����׽���Push��ջ�ṹ�У����������߳��Ǳ߿�����Pop����
			ConfigInfo.LocalData.Push(AcceptSocket);
			hThread = CreateThread(NULL, 0, PortTransferServer, (LPVOID)&ConfigInfo, 0, 0);
			CloseHandle(hThread);
		}
	error0:
		closesocket(CtrlSocket);
	error1:
		closesocket(serversockid);
	error2:
		closesocket(ctrlsockid);
		return false;
	}
}

// ˫������ת��
namespace TwoForwarding
{
	typedef struct
	{
		SOCKET s1;
		SOCKET s2;
	}Stu_sock;

	// ��socket�ĵ�ַ�ṹ
	bool bindAndFunlisten(SOCKET s, int port)
	{
		// ��ַ�ṹ
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_port = htons(port);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		char on = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		// �󶨵�ַ�ṹ
		if (bind(s, (const sockaddr *)&addr, sizeof(sockaddr))<0)
		{
			// cout << "[-] Socket bind error." << endl;
			return false;
		}

		//�����˿�
		if (listen(s, 5)<0)
		{
			// cout << "[-] Listen error." << endl;
			return false;
		}
		return true;
	}

	// ����ת������
	void datatrans(LPVOID data)
	{
		char host_slave[20] = { 0 };
		char host_hacker[20] = { 0 };
		Stu_sock *stuSock = (Stu_sock *)data;
		SOCKET s1 = stuSock->s1;     // ���ܵ���slave������
		SOCKET s2 = stuSock->s2;     // ���ͳ�ȥ��socket
		int sentacount1 = 0;

		sockaddr_in addr = { 0 };
		int sizeofaddr = sizeof(sockaddr);

		if (getpeername(s1, (sockaddr *)&addr, &sizeofaddr))
		{

			strcpy(host_slave, inet_ntoa(addr.sin_addr));
			int port_slave = ntohs(addr.sin_port);
		}
		if (getpeername(s2, (sockaddr *)&addr, &sizeofaddr))
		{

			strcpy(host_hacker, inet_ntoa(addr.sin_addr));
			int port_hacker = ntohs(addr.sin_port);
		}

		// cout << "[+] Start Transport (" << host_slave << "<-> " << host_hacker << ") ......" << endl;
		char RecvBuffer[20480] = { 0 };
		char SendBuffer[20480] = { 0 };

		fd_set  readfd; fd_set  writefd;
		timeval timeset;
		timeset.tv_sec = 300;
		timeset.tv_usec = 0;
		int maxfd = max(s1, s2) + 1;
		bool flag = false;
		int readsize;
		while (TRUE)
		{
			readsize = 0;
			FD_ZERO(&readfd);
			FD_ZERO(&writefd);
			FD_SET((UINT)s1, &readfd);
			FD_SET((UINT)s2, &readfd);
			FD_SET((UINT)s1, &writefd);
			FD_SET((UINT)s2, &writefd);

			int result = select(maxfd, &readfd, &writefd, NULL, &timeset);
			if (result<0 && (errno != EINTR))
			{
				// cout << "[-] Select error." << endl;
				break;
			}
			else if (result == 0)
			{
				// cout << "[-] Socket time out." << endl;
				break;
			}

			// û����û��ʱ
			if (FD_ISSET(s1, &readfd) && flag)
			{
				// if (totalread1<20408)
				{
					// ����host������
					readsize = recv(s1, RecvBuffer, 20480, 0);
					if (readsize == -1)
					{
						break;
					}
					/*
					if (readsize == SOCKET_ERROR || readsize == 0)
					{
					cout << "!!!" << endl;
					}
					*/
					memcpy(SendBuffer, RecvBuffer, readsize);
					memset(RecvBuffer, 0, 20480);
					printf("[ �հ� ] %d bytes \n", readsize);

				}
			}
			if (FD_ISSET(s1, &writefd) && flag && (readsize>0))
			{

				int sendsize = send(s2, SendBuffer, readsize, 0);//����slave
				if (sendsize == 0)
				{
					break;
				}
				if (sendsize<0 && (errno != EINTR))
				{
					// cout << "[-] Send to s2 unknow error." << endl;
					break;
				}

				memset(SendBuffer, 0, 20480);
				printf("[ ���� ] %d bytes \n", sendsize);

			}
			if (FD_ISSET(s2, &readfd) && (!flag))
			{
				{
					// ����slave��������
					readsize = recv(s2, RecvBuffer, 20480, 0);
					memcpy(SendBuffer, RecvBuffer, readsize);

					printf("[ �հ� ] %d bytes \n", readsize);

					// totalread1+=readsize;
					memset(RecvBuffer, 0, 20480);
				}
			}
			if (FD_ISSET(s1, &writefd) && (!flag) && (readsize>0))
			{
				// ����host
				readsize = send(s1, SendBuffer, readsize, 0);
				if (readsize == 0)
				{
					break;
				}
				if (readsize<0)
				{
					// cout << "[-] Send to s2 unknow error." << endl;
					break;
				}

				printf("[ ���� ] %d bytes \n", readsize);
				memset(SendBuffer, 0, 20480);
			}
			flag = !flag;
			Sleep(5);
		}
		closesocket(s1);
		closesocket(s2);
		// cout << "[+] OK! I Closed The Two Socket." << endl;
	}

	// ���ӷ����
	int client_connect(int sockfd, char* server, int port)
	{
		struct sockaddr_in cliaddr;
		struct hostent *host;

		// ���Զ��������IP
		if (!(host = gethostbyname(server)))
		{
			// printf("[-] Gethostbyname(%s) error:%s\n",server,strerror(errno));
			return(0);
		}

		// ����ַ�ṹ��ֵ
		memset(&cliaddr, 0, sizeof(struct sockaddr));
		cliaddr.sin_family = AF_INET;

		/*Զ�̶˿�*/
		cliaddr.sin_port = htons(port);
		cliaddr.sin_addr = *((struct in_addr *)host->h_addr);

		// ȥ����Զ������listen������
		if (connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr))<0)
		{
			// printf("[-] Connect error.\r\n");
			return(0);
		}
		return(1);
	}

	// slave����
	void Client(char* hostIp, char * slaveIp, int destionPort, int slavePort)
	{
		Stu_sock stu_sock;
		fd_set fd;
		char buffer[20480];
		int l;
		while (TRUE)
		{
			// �����׽���
			SOCKET sock1 = socket(AF_INET, SOCK_STREAM, 0);
			SOCKET sock2 = socket(AF_INET, SOCK_STREAM, 0);

			// cout << "[+] Make a Connection to " << hostIp << "on port:" << slaveIp << "...." << endl;
			if (sock1<0 || sock1<0)
			{
				// cout << "[-] Create socket error" << endl;
				return;
			}
			fflush(stdout);
			if (client_connect(sock1, hostIp, destionPort) == 0)
			{
				closesocket(sock1);
				closesocket(sock2);
				continue;/*�������ѭ��*/
			}

			memset(buffer, 0, 20480);
			while (1)
			{
				// ��sock����,����set��
				FD_ZERO(&fd);
				FD_SET(sock1, &fd);

				// select�¼�  �� д �쳣
				if (select(sock1 + 1, &fd, NULL, NULL, NULL) == SOCKET_ERROR)
				{
					if (errno == WSAEINTR) continue;
					break;
				}

				// FD_ISSET����ֵ>0 ��ʾSET��Ŀɶ�д
				if (FD_ISSET(sock1, &fd))
				{
					l = recv(sock1, buffer, 20480, 0);
					break;
				}
				Sleep(5);
			}

			if (l <= 0)
			{
				// cout << "[-] There is a error...Create a new connection." << endl;
				continue;
			}
			while (1)
			{
				// cout << "[+] Connect OK!    \n[+] xlcTeam congratulations!" << endl;
				// cout << "[+] Make a Connection to " << hostIp << "on port:" << slaveIp << "...." << endl;
				fflush(stdout);
				if (client_connect(sock2, slaveIp, slavePort) == 0)
				{
					closesocket(sock1);
					closesocket(sock2);
					continue;
				}

				if (send(sock2, buffer, l, 0) == SOCKET_ERROR)
				{
					// cout << "[-] Send failed." << endl;
					continue;
				}

				l = 0;
				memset(buffer, 0, 20480);
				break;
			}

			// cout << "[+] All Connect OK!" << endl;
			stu_sock.s1 = sock1;
			stu_sock.s2 = sock2;

			HANDLE  hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)datatrans, (LPVOID)&stu_sock, 0, NULL);
			if (hThread == NULL)
			{
				TerminateThread(hThread, 0);
				return;
			}
		}
	}

	// ���IP��ַ��ʽ�Ƿ���ȷ
	bool checkIP(char * str)
	{
		if (INADDR_NONE == inet_addr(str))
		{
			return FALSE;
		}
		return TRUE;
	}

	// �������ܺ���
	void Server(int port1, int port2)
	{
		Stu_sock stu_sock;

		// �����׽���
		SOCKET sock1 = socket(AF_INET, SOCK_STREAM, 0);
		SOCKET sock2 = socket(AF_INET, SOCK_STREAM, 0);
		if (sock1<0 || sock1<0)
		{
			// cout << "[-] Create socket error" << endl;
			return;
		}

		// �󶨶˿ڵ�socket������
		if (!bindAndFunlisten(sock1, port1) || !bindAndFunlisten(sock2, port2))
		{
			return;
		}
		// ���������˽�����
		int SizeOfAddr = sizeof(sockaddr);
		while (true)
		{
			// cout << "[+] Waiting for Client ......" << endl;
			sockaddr_in remoteSockAddr;

			// sock1�ȴ�����
			SOCKET  recvSock1 = accept(sock1, (sockaddr *)&remoteSockAddr, &SizeOfAddr);
			// cout << recvSock1 << endl;
			if (recvSock1<0)
			{
				// cout << "[-] Accept error." << endl;
				continue;
			}
			// cout << "[+] Accept a Client on port " << port1 << "  from " << inet_ntoa(remoteSockAddr.sin_addr) << endl;
			// cout << "[+] Waiting another Client on port:" << port2 << "...." << endl;

			SOCKET recvSock2 = accept(sock2, (sockaddr *)&remoteSockAddr, &SizeOfAddr);
			cout << recvSock2 << endl;
			if (recvSock2<0)
			{
				// cout << "[-] Accept error." << endl;
				continue;
			}
			// cout << "[+] Accept a Client on port" << port1 << "  from " << inet_ntoa(remoteSockAddr.sin_addr) << endl;

			// ��������������
			// cout << "[+] Accept Connect OK!" << endl;

			stu_sock.s1 = recvSock1;
			stu_sock.s2 = recvSock2;
			DWORD dwThreadID;

			// ����һ��ת�����ݵ��߳�
			HANDLE  hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)datatrans, (LPVOID)&stu_sock, 0, &dwThreadID);
			if (hThread == NULL)
			{
				// �̴߳���ֱ���˳�
				TerminateThread(hThread, 0);
				return;
			}
			// cout << "[+] CreateThread OK!" << endl;
			Sleep(800);// ����ǰ�߳�
		}
	}
}

int main(int argc, char *argv[])
{
	WSADATA wsa;
	if (!WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
	{
		return 0;
	}
	if (argc <= 1)
	{
		printf("  _____ _                 _____                                _ \n");
		printf(" |  ___| | _____      __ |  ___|__  _ ____      ____ _ _ __ __| |\n");
		printf(" | |_  | |/ _ \ \ /\ / / | |_ / _ \| '__\ \ /\ / / _` | '__/ _` |\n");
		printf(" |  _| | | (_) \ V  V /  |  _| (_) | |   \ V  V / (_| | | | (_| |\n");
		printf(" |_|   |_|\___/ \_/\_/   |_|  \___/|_|    \_/\_/ \__,_|_|  \__,_|\n");
		printf("                                                                 \n\n");
		printf("E-Mail: me@lyshark.com \n\n");
		printf("Usage: \n\n");
		printf("\t ListenPort          ָ��ת�������������˿� \n");
		printf("\t RemoteAddress       ָ���Զ�IP��ַ \n");
		printf("\t RemotePort          ָ���Զ˶˿ں� \n");
		printf("\t LocalPort           ָ�����˶˿ں� \n");
		printf("\t ServerAddress       ָ�������IP��ַ \n");
		printf("\t ServerPort          ָ��������Port�˿ں� \n");
		printf("\t ConnectAddress      ָ�����ӵ��Զ�IP��ַ \n");
		printf("\t ConnectPort         ָ���Զ�����Port�˿ں� \n\n");
	}
	if (argc == 8)
	{
		if (strcmp(argv[1], "Forward") == 0 &&
			strcmp(argv[2], "--ListenPort") == 0 &&
			strcmp(argv[4], "--RemoteAddress") == 0 &&
			strcmp(argv[6], "--RemotePort") == 0
			)
		{
			printf("[*] �������ģʽ \n");
			printf("[+] ���������˿�: %s \n", argv[3]);
			printf("[+] ����ת����ַ %s:%s \n\n", argv[5], argv[7]);
			ForwardForwarding::Forwarding(atoi(argv[3]), argv[5], atoi(argv[7]));
		}
	}
	if (argc == 10)
	{
		if (strcmp(argv[1], "ReverseClient") == 0 &&
			strcmp(argv[2], "--ServerAddress") == 0 &&
			strcmp(argv[4], "--ServerPort") == 0 &&
			strcmp(argv[6], "--ConnectAddress") == 0 &&
			strcmp(argv[8], "--ConnectPort") == 0
			)
		{
			printf("[*] �����������ģʽ (�ͻ���) \n");
			printf("[+] ����˵�ַ %s:%s \n", argv[3], argv[5]);
			printf("[+] ����������ַ %s:%s \n", argv[7], argv[9]);

			ReverseForwarding::PortTransferClient(argv[3], atoi(argv[5]), argv[7], atoi(argv[9]));
		}
	}
	if (argc == 6)
	{
		if (strcmp(argv[1], "ReverseServer") == 0 &&
			strcmp(argv[2], "--ListenPort") == 0 &&
			strcmp(argv[4], "--LocalPort") == 0
			)
		{
			printf("[*] �����������ģʽ (�����) \n");
			printf("[+] �����˿�: %s \n", argv[3]);
			printf("[+] �������ӵ�ַ: localhost:%s \n\n", argv[5]);

			ReverseForwarding::PortTransferServer(atoi(argv[3]), atoi(argv[5]));
		}
	}
	if (argc == 6)
	{
		if (strcmp(argv[1], "TwoForwardServer") == 0 &&
			strcmp(argv[2], "--ListenPort") == 0 &&
			strcmp(argv[4], "--LocalPort") == 0
			)
		{
			printf("[*] ˫�����ת��ģʽ (�����) \n");
			printf("[+] �����˿�: %s \n", argv[3]);
			printf("[+] �������ӵ�ַ: localhost:%s \n\n", argv[5]);

			TwoForwarding::Server(atoi(argv[3]), atoi(argv[5]));
		}
	}
	if (argc == 10)
	{
		if (strcmp(argv[1], "TwoForwardClient") == 0 &&
			strcmp(argv[2], "--ServerAddress") == 0 &&
			strcmp(argv[4], "--ServerPort") == 0 &&
			strcmp(argv[6], "--ConnectAddress") == 0 &&
			strcmp(argv[8], "--ConnectPort") == 0
			)
		{
			printf("[*] ˫�����ת��ģʽ (�ͻ���) \n");
			printf("[+] ����˵�ַ %s:%s \n", argv[3], argv[5]);
			printf("[+] ����������ַ %s:%s \n", argv[7], argv[9]);
			TwoForwarding::Client(argv[3], argv[7], atoi(argv[5]), atoi(argv[9]));
		}
	}

	WSACleanup();
	return 0;
}