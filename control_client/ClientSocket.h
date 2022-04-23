#pragma once
#include "pch.h"
#include "framework.h"
#include "string"
#include <vector>

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

typedef struct MouseEvent { //����¼��ṹ��
	MouseEvent() { //��ʼ ���캯��
		nAction = 0;
		nButtom = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//��������  ��� �ƶ� ˫��
	WORD nButtom;//��� �Ҽ� �м� 0 2 1
	POINT ptXY;//����
}MOUSEEVENT, * PMOUSEEVENT;

typedef struct file_info {
	BOOL IsInvalid; //�Ƿ�����ЧĿ¼
	char szFileName[256];//�ļ�����
	BOOL IsDirectory;//��Ŀ¼�����ļ� 0:��Ŀ¼ 1:����Ŀ¼
	BOOL HasNext;//�ļ������Ƿ��к��� 0:û�� 1:��
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
}FILEINFO, * PFILEINFO;

std::string GetErrorinfo(int wsaErrCode);//��������� //TODO:ѧϰ


class CClientSocket
{
public:
	//��̬����
	static CClientSocket* getInstance()
	{
		if (m_instance == NULL) //��̬����û��thisָ�� , �޷�ֱ�ӷ��ʳ�Ա����
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket(int nIP, int nPort)
	{
		// socket bind listen accept read write close
			//server;
		if (m_sock != INVALID_SOCKET)  CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		//TODO:У��
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(nIP);//�����ֽ���ת����·�ֽ���
		serv_addr.sin_port = htons(nPort);//default port: 8848
		if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("ָ����IP������!");
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == -1) {
			AfxMessageBox("����ʧ��!");
			TRACE("����ʧ�� : %d %s\r\n", WSAGetLastError(), GetErrorinfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;

	}
	
#define BUFFER_SIZE 4096
	int DealCommand() // �������� from client 
	{
		if (m_sock == -1) return -1;
		//char buffer[1024] = "";
		//��������װ��
		char* buffer = m_buffer.data();
		//TODO: Ϊʲô����delete buffer ? ������������ ������ ���ܻᷢ�Ͷ������ ���ǵ��� ���Ƕ��
		//Ϊʲô��Ҫ��Ա�������������ð�? ����˻�Ӧ������ ���������˷���ģʽ ����TCPÿ�η�������Ӧ��,��ô�����������кܶ����,���Ƕ�ȡһ��������ͻᶪ��
		if (buffer == NULL) {
			TRACE("�ڴ治��! server buffer !\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{//
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				//delete[]buffer;
				return -1;
			}
			index += len;
			len = index;
			//TODO:��������
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				//delete[]buffer;
				return m_packet.sCmd;
			}
		}
		//delete[]buffer;
		return -1;
	}
	bool Send(char* const pData, int nSize)
	{
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		TRACE("m_sock : %d\r\n", m_sock);
		if (m_sock == -1) return false;
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
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
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

private:
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {} //ss = server_scoket ����
	CClientSocket(const CClientSocket& ss)
	{
		m_sock = ss.m_sock;
	}
	CClientSocket() //��������
	{
		//m_sock = INVALID_SOCKET;//-1
		if (InitSocketEnv() == FALSE)
		{
			MessageBox(NULL, _T("�׽��ֳ�ʼ��ʧ��!"), _T("��ʼ������!"), MB_OK | MB_ICONERROR); // MFC
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		//m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CClientSocket() //��������
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
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CClientSocket* m_instance; //����ʵ��

	class CHelper
	{
	public:
		CHelper()
		{
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}

	private:

	};
	static CHelper m_helper;
};

//extern CServerSocket server;


