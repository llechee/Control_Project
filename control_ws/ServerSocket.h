#pragma once
#include "pch.h"
#include "framework.h"
class CServerSocket
{
public:
	//静态函数
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL) //静态函数没有this指针 , 无法直接访问成员变量
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket()
	{
		// socket bind listen accept read write close
			//server;

		//TODO:校验
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(8848);//default port: 8848
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
		if (listen(m_sock, 10) == -1) return false;
		return true;
		
	}
	bool AcceptClient()
	{
		sockaddr_in client_addr;
		//char buffer[1024] = "";
		int client_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &client_sz);
		if (m_client == -1)  return false;
		return true;
		//recv(client, buffer, sizeof(buffer), 0);
		//send(client, buffer, sizeof(buffer), 0);
		
	}
	int DealCommand()
	{
		if (m_client == -1) return false;
		char buffer[1024] = "";
		while (true)
		{
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0)
			{
				return -1;
			}
			//TODO:处理命令
		}
	}
	bool Send(char* const pData,int nSize)
	{
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}


private:
	SOCKET m_sock,m_client;
	CServerSocket& operator=(const CServerSocket& ss) {} //ss = server_scoket 重载
	CServerSocket(const CServerSocket& ss) 
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() //析构函数
	{
		//m_sock = INVALID_SOCKET;//-1
		m_client = INVALID_SOCKET;
		if (InitSocketEnv() == FALSE)
		{
			MessageBox(NULL, _T("套接字初始化失败!"), _T("初始化错误!"), MB_OK | MB_ICONERROR); // MFC
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() //析构函数
	{
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSocketEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//windows socket init  // TODO:返回值处理
		{
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance; //声明实例

	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	
	private:

	};
	static CHelper m_helper;
};

//extern CServerSocket server;

