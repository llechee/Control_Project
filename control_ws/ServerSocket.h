#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
class CPacket // ����
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {} //���� ��ʼ��

	//����һ������ ������������ָ�� �����������ݴ�С
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) { //��� ��ʼ��
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack) { //��� ��ʼ��
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	//���ݰ������������е����ݷ��䵽��Ա������
	//����һ������ָ��  �����������ݴ�С
	CPacket(const BYTE* pData, size_t& nSize) //���������� nsize:���������ֽ� pdata:
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // �ҵ���ͷ
			{
				sHead = *(WORD*)(pData + i);//��ȡ��ͷ
				i += 2; //��ֹ������� 
				break;
			}
		}
		//TODO: >= : =
		if (i + 4 + 2 + 2 > nSize) // �жϽ����Ƿ�ɹ� baotou4 length commed sum 422  �����ݿ��ܲ�ȫ,���߰�ͷδ��ȫ������
		{	//i����������ڵ������� ����ͷ+2 ������+4 ��commond+2 ��data+nlength-4 ��sum+2  
			//Ϊʲô��i ǰ���п����з����� ��Ҫ�Ѻ���İ��Ƶ�bufferǰ��(bufferǰ���ǲ�Ҫ������)
			nSize = 0;
			return;
		}
		nLength = *(WORD*)(pData + i); i += 4;//��ȡ����
		if (nLength + i > nSize) { //��δ��ȫ�յ�,����ʧ�� ,����
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;//��ȡ������
		if (nLength > 4) {				   //���ݵĳ��ȴ�����
			strData.resize(nLength - 2 - 2); // ��ȥsum��cmd -2-2
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);//��ȡ����
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2; //i����size  //����У����
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j] & 0xFF);
		}
		if (sum == sSum) {
			nSize = i; // head length data 2 4  //ͷ+����+����
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
	int Size() { //��ð����ݵĴ�С
		return nLength + 6;//ͷ��2���ֽ� ���Լ����ֽ�
	}
	const char* Data() {//���ڽ������������������뻺����
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str(); //ָ��ָ���ͷ
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str(); //����������
	}

public:
	WORD sHead;// head packet
	DWORD nLength; //������ (�ӿ������ʼ����У�������  4�ֽ�
	WORD sCmd; //�������� 2�ֽ�
	std::string strData; //������
	WORD sSum; //��У��
	std::string strOut;//������������
};
#pragma pack(pop)

typedef struct MouseEvent{ //����¼��ṹ��
	MouseEvent() { //��ʼ ���캯��
		nAction = 0;
		nButtom = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//��������  ��� �ƶ� ˫��
	WORD nButtom;//��� �Ҽ� �м� 0 2 1
	POINT ptXY;//����
}MOUSEEVENT,*PMOUSEEVENT;

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
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9537);//default port: 8848
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
		if (listen(m_sock, 1) == -1) return false;
		return true;

	}
	bool AcceptClient()
	{
		TRACE("Begin Accept!\r\n");
		sockaddr_in client_addr;
		//char buffer[1024] = "";
		int client_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &client_sz);//�����׽���
		TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1)  return false;
		return true;
		//recv(client, buffer, sizeof(buffer), 0);
		//send(client, buffer, sizeof(buffer), 0);

	}
#define BUFFER_SIZE 4096
	int DealCommand() // �������� from client 
	{
		if (m_client == -1) return -1;
		//��������װ��
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("�ڴ治��! server buffer !\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{//
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			//len += 2;//TODO: delete
			if (len <= 0)
			{
				delete[]buffer;
				return -1;
			}
			TRACE("recv len : %d\r\n", len);
			index += len;
			len = index;
			//TODO:��������
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}
	bool Send(char* const pData, int nSize)
	{
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath) {//��ȡ�ļ�·��(�б�)
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) { //scmd�������ֽ�
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEVENT& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEVENT));
			return true;
		}
		return false;
	}
	CPacket& GetPacket() {//�õ�cmd
		return m_packet;
	}
	void CloseClient()
	{
		closesocket(m_client);
		m_client = INVALID_SOCKET;
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

