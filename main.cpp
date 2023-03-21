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

// 正向转发
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

	// 定义一个模板类 STACK
	template<typename T>
	class STACK
	{
	private:
		// 私有成员变量
		int top;            // 栈顶元素的索引
		T Data[MAXSTACK];   // 用于存储栈元素的数组

	public:
		// 公有成员函数

		// 构造函数
		STACK()
		{
			top = -1;       // 将栈顶索引初始化为 -1，表示栈为空
		}

		// 判断栈是否为空的函数
		bool IsEmpty()
		{
			return top < 0;  // 如果栈顶索引小于 0，则栈为空
		}

		// 判断栈是否已满的函数
		bool IsFull()
		{
			return top >= MAXSTACK;  // 如果栈顶索引大于等于 MAXSTACK，则栈已满
		}

		// 将元素入栈的函数
		bool Push(T data)
		{
			if (IsFull())
				return false;   // 如果栈已满，则入栈失败
			top++;              // 栈顶索引加 1
			Data[top] = data;   // 将元素存入栈顶
			return true;        // 入栈成功
		}

		// 将元素出栈的函数
		T Pop()
		{
			return Data[top--]; // 将栈顶元素弹出，并将栈顶索引减 1
		}
	};

	// 定义一个模板类 TransferParam，包含两个模板类型参数 X 和 Y
	template<typename X, typename Y>
	class TransferParam
	{
	public:
		// 公有成员变量
		X GlobalData;           // 全局数据，类型为 X
		STACK<Y> LocalData;     // 局部数据，类型为 STACK<Y>

	public:
		// 公有成员函数

		// 默认构造函数
		TransferParam();

		// 将数据压入局部栈的函数
		bool Push(Y data);

		// 从局部栈中弹出数据的函数
		Y Pop();
	};

	// TransferParam 模板类的构造函数
	template<typename X, typename Y>
	TransferParam<X, Y>::TransferParam()
	{
		memset(this, 0, sizeof(TransferParam)); // 将该对象内存空间清零
	}

	// 向局部栈中压入数据的函数
	template<typename X, typename Y>
	bool TransferParam<X, Y>::Push(Y data)
	{
		return LocalData.Push(data); // 调用 STACK<Y> 类型的 LocalData 对象的 Push 函数
	}

	// 从局部栈中弹出数据的函数
	template<typename X, typename Y>
	Y TransferParam<X, Y>::Pop()
	{
		return LocalData.Pop(data); // 调用 STACK<Y> 类型的 LocalData 对象的 Pop 函数
	}

	// 将DataBuf中的DataLen个字节发到套接字s上面
	int DataSend(SOCKET s, char *DataBuf, int DataLen)
	{
		int nBytesLeft = DataLen;    // 剩余未发送的字节数
		int nBytesSent = 0;          // 已发送的字节数
		int ret;                     // send 函数的返回值
		int iMode = 0;              // 非阻塞模式
		ioctlsocket(s, FIONBIO, (u_long FAR*) &iMode);  // 设置套接字为非阻塞模式
		while (nBytesLeft > 0)
		{
			ret = send(s, DataBuf + nBytesSent, nBytesLeft, 0);  // 发送数据

			printf("[ 发包 ] %d bytes \n", strlen(DataBuf));    // 输出已发送数据的长度
			if (ret <= 0)
			{
				break;
			}
			nBytesSent += ret;
			nBytesLeft -= ret;
		}
		return nBytesSent;  // 返回已发送的字节数
	}

	// 线程函数在两个SOCKET中进行数据转发
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
				printf("[ 收包 ] %d bytes \n", strlen(RecvBuf));

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
				printf("[ 收包 ] %d bytes \n", strlen(RecvBuf));
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

	// 封装过的连接指定的IP和端口
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

	// 在dwIP上绑定wPort端口
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

	// 执行端口映射函数
	DWORD WINAPI PortTransfer(LPVOID lParam)
	{
		TransferParam<ADDRESS, SOCKET> *ConfigInfo = (TransferParam<ADDRESS, SOCKET>*)lParam;
		SOCKET ClientSock, ServerSock;

		// 出栈获得客户的套接字
		ClientSock = ConfigInfo->LocalData.Pop();

		// 先连接到目标计算机的服务
		ServerSock = ConnectHost(ConfigInfo->GlobalData.szIP, ConfigInfo->GlobalData.wPort);
		if (ServerSock <= 0)
		{
			closesocket(ClientSock);
			return 0;
		}

		SOCKINFO socks;
		socks.ClientSock = ClientSock;       // 客户的套接字
		socks.ServerSock = ServerSock;       // 目标计算机服务的套接字
		return TransmitData((LPVOID)&socks); // 进入数据转发状态
	}

	// 转发函数
	BOOL Forwarding(WORD ListenPort, PCHAR szIP, WORD wPort)
	{
		// 执行映射函数
		HANDLE hThread;
		SOCKET AcceptSocket;
		TransferParam<ADDRESS, SOCKET> ConfigInfo;

		// 填充地址
		_snprintf(ConfigInfo.GlobalData.szIP, 32, "%s", szIP);
		ConfigInfo.GlobalData.wPort = wPort;

		// 监听服务端口 即映射端口
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

			// 将接受到的客户请求套接字转到新的线程里处理
			ConfigInfo.LocalData.Push(AcceptSocket);
			hThread = CreateThread(0, 0, PortTransfer, (LPVOID)&ConfigInfo, 0, 0);
			CloseHandle(hThread);
		}
	}
}

// 反向端口转发
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
			printf("[ 发包 ] %d bytes \n", strlen(DataBuf));
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
				printf("[ 收包 ] %d bytes \n", strlen(RecvBuf));
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
				printf("[ 收包 ] %d bytes \n", strlen(RecvBuf));
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

	// 创建一个临时的套接字,指针wPort获得创建的临时端口
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

	// 端口转发客户端
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

		// 获得运行PortTransfer_3模式的计算机的IP
		dwCtrlIP = clientaddr.sin_addr.S_un.S_addr;
		// wPort = ntohs(clientaddr.sin_port);

		SOCKET ClientSocket, ServerSocket;
		SOCKINFO socks;

		// 向公网建立新的连接
		ClientSocket = ConnectHost(dwCtrlIP, ConfigInfo->LocalData.Pop());
		if (ClientSocket <= 0)
		{
			return 0;
		}

		// 连接到目标计算机的服务
		ServerSocket = ConnectHost(ConfigInfo->GlobalData.szIP, ConfigInfo->GlobalData.wPort);
		if (ServerSocket <= 0)
		{
			closesocket(ClientSocket);
			return 0;
		}

		// 转发数据包
		socks.ClientSock = ClientSocket;    // 公网计算机的套接字
		socks.ServerSock = ServerSocket;    // 目标计算机服务的套接字
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

		// 与PortTransfer_3模式（工作在公网）的计算机建立控制管道连接
		SOCKET CtrlSocket = ConnectHost(szCtrlIP, wCtrlPort);
		if (CtrlSocket <= 0)
		{
			goto error;
		}

		ConfigInfo.GlobalData.s = CtrlSocket;
		while (1)
		{
			// 接收来自公网计算机的命令，数据为一个WORD，表示公网计算机监听了这个端口
			nRecv = recv(CtrlSocket, (char*)&ReqPort, sizeof(ReqPort), 0);
			if (nRecv <= 0)
			{
				goto error;
			}

			// 传递信息的结构
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

	// 端口转发服务端
	DWORD WINAPI PortTransferServer(LPVOID lParam)
	{
		SOCKINFO socks;
		SOCKET ClientSocket, ServerSocket, CtrlSocket, tmpSocket;
		TransferParam<SOCKET, SOCKET> *ConfigInfo = (TransferParam<SOCKET, SOCKET>*)lParam;
		CtrlSocket = ConfigInfo->GlobalData;
		ClientSocket = ConfigInfo->LocalData.Pop();
		WORD wPort;

		// 创建临时端口
		tmpSocket = CreateTmpSocket(&wPort);
		if (tmpSocket <= 0 || wPort <= 0)
		{
			closesocket(ClientSocket);
			return 0;
		}

		// 通知内网用户发起新的连接到刚创建的临时端口
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

		// 开始转发
		socks.ClientSock = ClientSocket;
		socks.ServerSock = ServerSocket;
		return TransmitData((LPVOID)&socks);
	}

	// 监听的两个端口
	BOOL PortTransferServer(WORD wCtrlPort, WORD wServerPort)
	{
		HANDLE hThread;
		BOOL bOptVal = TRUE;
		int bOptLen = sizeof(BOOL);
		TransferParam<SOCKET, SOCKET> ConfigInfo;
		SOCKET ctrlsockid, serversockid, CtrlSocket, AcceptSocket;

		// 创建套接字
		ctrlsockid = CreateSocket(INADDR_ANY, wCtrlPort);
		if (ctrlsockid <= 0)
		{
			goto error2;
		}

		// 创建套接字
		serversockid = CreateSocket(INADDR_ANY, wServerPort);
		if (serversockid <= 0)
		{
			goto error1;
		}

		// 接受来自（内网用户发起）PortTransfer_2模式建立控制管道连接的请求
		CtrlSocket = accept(ctrlsockid, NULL, NULL);
		if (CtrlSocket == INVALID_SOCKET)
		{
			goto error0;
		}

		if (setsockopt(CtrlSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&bOptVal, bOptLen) == SOCKET_ERROR)
		{
			goto error0;
		}

		// 与内网用户建立了连接后就相当端口映射成功了
		// 准备进入接收服务请求状态，并将在新起的线程中通过控制管道通知内网用户发起新的连接进行数据转发
		ConfigInfo.GlobalData = CtrlSocket;
		while (1)
		{
			// 接受新客户
			AcceptSocket = accept(serversockid, NULL, NULL);
			if (AcceptSocket == INVALID_SOCKET)
			{
				Sleep(1000);
				continue;
			}

			// 把接受到的套接字Push到栈结构中，传到新起线程那边可以再Pop出来
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

// 双向流量转发
namespace TwoForwarding
{
	typedef struct
	{
		SOCKET s1;
		SOCKET s2;
	}Stu_sock;

	// 绑定socket的地址结构
	bool bindAndFunlisten(SOCKET s, int port)
	{
		// 地址结构
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_port = htons(port);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		char on = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		// 绑定地址结构
		if (bind(s, (const sockaddr *)&addr, sizeof(sockaddr))<0)
		{
			// cout << "[-] Socket bind error." << endl;
			return false;
		}

		//监听端口
		if (listen(s, 5)<0)
		{
			// cout << "[-] Listen error." << endl;
			return false;
		}
		return true;
	}

	// 数据转发函数
	void datatrans(LPVOID data)
	{
		char host_slave[20] = { 0 };
		char host_hacker[20] = { 0 };
		Stu_sock *stuSock = (Stu_sock *)data;
		SOCKET s1 = stuSock->s1;     // 接受的是slave的数据
		SOCKET s2 = stuSock->s2;     // 发送出去的socket
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

			// 没出错没超时
			if (FD_ISSET(s1, &readfd) && flag)
			{
				// if (totalread1<20408)
				{
					// 接受host的请求
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
					printf("[ 收包 ] %d bytes \n", readsize);

				}
			}
			if (FD_ISSET(s1, &writefd) && flag && (readsize>0))
			{

				int sendsize = send(s2, SendBuffer, readsize, 0);//发给slave
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
				printf("[ 发包 ] %d bytes \n", sendsize);

			}
			if (FD_ISSET(s2, &readfd) && (!flag))
			{
				{
					// 接受slave返回数据
					readsize = recv(s2, RecvBuffer, 20480, 0);
					memcpy(SendBuffer, RecvBuffer, readsize);

					printf("[ 收包 ] %d bytes \n", readsize);

					// totalread1+=readsize;
					memset(RecvBuffer, 0, 20480);
				}
			}
			if (FD_ISSET(s1, &writefd) && (!flag) && (readsize>0))
			{
				// 发给host
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

				printf("[ 发包 ] %d bytes \n", readsize);
				memset(SendBuffer, 0, 20480);
			}
			flag = !flag;
			Sleep(5);
		}
		closesocket(s1);
		closesocket(s2);
		// cout << "[+] OK! I Closed The Two Socket." << endl;
	}

	// 连接服务端
	int client_connect(int sockfd, char* server, int port)
	{
		struct sockaddr_in cliaddr;
		struct hostent *host;

		// 获得远程主机的IP
		if (!(host = gethostbyname(server)))
		{
			// printf("[-] Gethostbyname(%s) error:%s\n",server,strerror(errno));
			return(0);
		}

		// 给地址结构赋值
		memset(&cliaddr, 0, sizeof(struct sockaddr));
		cliaddr.sin_family = AF_INET;

		/*远程端口*/
		cliaddr.sin_port = htons(port);
		cliaddr.sin_addr = *((struct in_addr *)host->h_addr);

		// 去连接远程正在listen的主机
		if (connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr))<0)
		{
			// printf("[-] Connect error.\r\n");
			return(0);
		}
		return(1);
	}

	// slave函数
	void Client(char* hostIp, char * slaveIp, int destionPort, int slavePort)
	{
		Stu_sock stu_sock;
		fd_set fd;
		char buffer[20480];
		int l;
		while (TRUE)
		{
			// 创建套接字
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
				continue;/*跳过这次循环*/
			}

			memset(buffer, 0, 20480);
			while (1)
			{
				// 把sock清零,加入set集
				FD_ZERO(&fd);
				FD_SET(sock1, &fd);

				// select事件  读 写 异常
				if (select(sock1 + 1, &fd, NULL, NULL, NULL) == SOCKET_ERROR)
				{
					if (errno == WSAEINTR) continue;
					break;
				}

				// FD_ISSET返回值>0 表示SET里的可读写
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

	// 检查IP地址格式是否正确
	bool checkIP(char * str)
	{
		if (INADDR_NONE == inet_addr(str))
		{
			return FALSE;
		}
		return TRUE;
	}

	// 监听功能函数
	void Server(int port1, int port2)
	{
		Stu_sock stu_sock;

		// 创建套接字
		SOCKET sock1 = socket(AF_INET, SOCK_STREAM, 0);
		SOCKET sock2 = socket(AF_INET, SOCK_STREAM, 0);
		if (sock1<0 || sock1<0)
		{
			// cout << "[-] Create socket error" << endl;
			return;
		}

		// 绑定端口到socket并监听
		if (!bindAndFunlisten(sock1, port1) || !bindAndFunlisten(sock2, port2))
		{
			return;
		}
		// 都监听好了接下来
		int SizeOfAddr = sizeof(sockaddr);
		while (true)
		{
			// cout << "[+] Waiting for Client ......" << endl;
			sockaddr_in remoteSockAddr;

			// sock1等待连接
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

			// 两个都连上来了
			// cout << "[+] Accept Connect OK!" << endl;

			stu_sock.s1 = recvSock1;
			stu_sock.s2 = recvSock2;
			DWORD dwThreadID;

			// 创建一个转发数据的线程
			HANDLE  hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)datatrans, (LPVOID)&stu_sock, 0, &dwThreadID);
			if (hThread == NULL)
			{
				// 线程错了直接退出
				TerminateThread(hThread, 0);
				return;
			}
			// cout << "[+] CreateThread OK!" << endl;
			Sleep(800);// 挂起当前线程
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
		printf("\t ListenPort          指定转发服务器侦听端口 \n");
		printf("\t RemoteAddress       指定对端IP地址 \n");
		printf("\t RemotePort          指定对端端口号 \n");
		printf("\t LocalPort           指定本端端口号 \n");
		printf("\t ServerAddress       指定服务端IP地址 \n");
		printf("\t ServerPort          指定服务器Port端口号 \n");
		printf("\t ConnectAddress      指定连接到对端IP地址 \n");
		printf("\t ConnectPort         指定对端连接Port端口号 \n\n");
	}
	if (argc == 8)
	{
		if (strcmp(argv[1], "Forward") == 0 &&
			strcmp(argv[2], "--ListenPort") == 0 &&
			strcmp(argv[4], "--RemoteAddress") == 0 &&
			strcmp(argv[6], "--RemotePort") == 0
			)
		{
			printf("[*] 正向隧道模式 \n");
			printf("[+] 本机侦听端口: %s \n", argv[3]);
			printf("[+] 流量转发地址 %s:%s \n\n", argv[5], argv[7]);
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
			printf("[*] 反向纯流量隧道模式 (客户端) \n");
			printf("[+] 服务端地址 %s:%s \n", argv[3], argv[5]);
			printf("[+] 连接内网地址 %s:%s \n", argv[7], argv[9]);

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
			printf("[*] 反向纯流量隧道模式 (服务端) \n");
			printf("[+] 侦听端口: %s \n", argv[3]);
			printf("[+] 本机连接地址: localhost:%s \n\n", argv[5]);

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
			printf("[*] 双向隧道转发模式 (服务端) \n");
			printf("[+] 侦听端口: %s \n", argv[3]);
			printf("[+] 本机连接地址: localhost:%s \n\n", argv[5]);

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
			printf("[*] 双向隧道转发模式 (客户端) \n");
			printf("[+] 服务端地址 %s:%s \n", argv[3], argv[5]);
			printf("[+] 连接内网地址 %s:%s \n", argv[7], argv[9]);
			TwoForwarding::Client(argv[3], argv[7], atoi(argv[5]), atoi(argv[9]));
		}
	}

	WSACleanup();
	return 0;
}