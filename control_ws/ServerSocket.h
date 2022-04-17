#pragma once
#include "pch.h"
#include "framework.h"
class CServerSocket
{
public:
	//��̬����
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL) //��̬����û��thisָ�� , �޷�ֱ�ӷ��ʳ�Ա����
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket()
	{
		// socket bind listen accept read write close
			//server;

		//TODO:У��
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
			//TODO:��������
		}
	}
	bool Send(char* const pData,int nSize)
	{
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}


private:
	SOCKET m_sock,m_client;
	CServerSocket& operator=(const CServerSocket& ss) {} //ss = server_scoket ����
	CServerSocket(const CServerSocket& ss) 
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() //��������
	{
		//m_sock = INVALID_SOCKET;//-1
		m_client = INVALID_SOCKET;
		if (InitSocketEnv() == FALSE)
		{
			MessageBox(NULL, _T("�׽��ֳ�ʼ��ʧ��!"), _T("��ʼ������!"), MB_OK | MB_ICONERROR); // MFC
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() //��������
	{
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSocketEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//windows socket init  // TODO:����ֵ����
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
	static CServerSocket* m_instance; //����ʵ��

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

