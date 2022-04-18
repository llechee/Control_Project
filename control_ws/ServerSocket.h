#pragma once
#include "pch.h"
#include "framework.h"

class CPacket // ����
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) //���������� nsize:���������ֽ� pdata:
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // �ҵ���ͷ
			{
				sHead = *(WORD*)(pData + i);
				i += 2; //��ֹ������� 
				break;
			}
		}
		if (i + 4 + 2 + 2 >= nSize) // �жϽ����Ƿ�ɹ� baotou4 length commed sum 422  �����ݿ��ܲ�ȫ,���߰�ͷδ��ȫ������
		{	//i����������ڵ������� ����ͷ+2 ������+4 ��commond+2 ��data+nlength-4 ��sum+2  
			//Ϊʲô��i ǰ���п����з����� ��Ҫ�Ѻ���İ��Ƶ�bufferǰ��(bufferǰ���ǲ�Ҫ������)
			nSize = 0;
			return;
		}
		nLength = *(WORD*)(pData + i); i += 4;
		if (nLength + i > nSize) { //��δ��ȫ�յ�,����ʧ�� ,����
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); // ��ȥsum��cmd -2-2
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2; //i����size
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[i] & 0xFF);
		}
		if (sum == sSum) {
			nSize = i; // head length data 2 4 
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	WORD sHead;// head packet
	DWORD nLength; //������
	WORD sCmd; //��������
	std::string strData; //������
	WORD sSum; //��У��
};

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
#define BUFFER_SIZE 4096
	int DealCommand() // �������� from client 
	{
		if (m_client == -1) return -1;
		//char buffer[1024] = "";
		//��������װ��
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{//
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				return -1;
			}
			index += len;
			len = index;
			//TODO:��������
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	bool Send(char* const pData, int nSize)
	{
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}


private:
	SOCKET m_sock, m_client;
	CPacket m_packet;
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

