#pragma once
#include "pch.h"
#include "framework.h"
#include "string"
#include <vector>

#pragma pack(push)
#pragma pack(1)
class CPacket // 包类
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {} //构造 初始化

	//参数一：命令 参数二：数据指针 参数三：数据大小
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) { //打包 初始化
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
	CPacket(const CPacket& pack) { //解包 初始化
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	//数据包解析，将包中的数据分配到成员变量中
	//参数一：数据指针  参数二：数据大小
	CPacket(const BYTE* pData, size_t& nSize) //解析数据用 nsize:发过来的字节 pdata:
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // 找到包头
			{
				sHead = *(WORD*)(pData + i);//提取包头
				i += 2; //防止特殊情况 
				break;
			}
		}
		//TODO: >= : =
		if (i + 4 + 2 + 2 > nSize) // 判断解析是否成功 baotou4 length commed sum 422  包数据可能不全,或者包头未能全部接收
		{	//i用来标记现在到哪里了 读包头+2 读长度+4 读commond+2 读data+nlength-4 读sum+2  
			//为什么用i 前面有可能有废数据 需要把后面的包移到buffer前面(buffer前面是不要的数据)
			nSize = 0;
			return;
		}
		nLength = *(WORD*)(pData + i); i += 4;//读取长度
		if (nLength + i > nSize) { //包未完全收到,解析失败 ,返回
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;//读取命令行
		if (nLength > 4) {				   //数据的长度大于四
			strData.resize(nLength - 2 - 2); // 减去sum和cmd -2-2
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);//读取数据
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2; //i就是size  //接收校验码
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j] & 0xFF);
		}
		if (sum == sSum) {
			nSize = i; // head length data 2 4  //头+长度+数据
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
	int Size() { //获得包数据的大小
		return nLength + 6;//头是2个字节 加自己四字节
	}
	const char* Data() {//用于解析包，将包数据送入缓冲区
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str(); //指针指向包头
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str(); //返回整个包
	}

public:
	WORD sHead;// head packet
	DWORD nLength; //包长度 (从控制命令开始到和校验结束）  4字节
	WORD sCmd; //控制命令 2字节
	std::string strData; //包数据
	WORD sSum; //和校验
	std::string strOut;//整个包的数据
};
#pragma pack(pop)

typedef struct MouseEvent { //鼠标事件结构体
	MouseEvent() { //初始 构造函数
		nAction = 0;
		nButtom = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//描述动作  点击 移动 双击
	WORD nButtom;//左键 右键 中键 0 2 1
	POINT ptXY;//坐标
}MOUSEEVENT, * PMOUSEEVENT;

typedef struct file_info {
	BOOL IsInvalid; //是否是有效目录
	char szFileName[256];//文件名字
	BOOL IsDirectory;//是目录还是文件 0:是目录 1:不是目录
	BOOL HasNext;//文件接收是否还有后续 0:没有 1:有
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
}FILEINFO, * PFILEINFO;

std::string GetErrorinfo(int wsaErrCode);//网络错误码 //TODO:学习


class CClientSocket
{
public:
	//静态函数
	static CClientSocket* getInstance()
	{
		if (m_instance == NULL) //静态函数没有this指针 , 无法直接访问成员变量
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
		//TODO:校验
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(nIP);//主机字节序转成网路字节序
		serv_addr.sin_port = htons(nPort);//default port: 8848
		if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定的IP不存在!");
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == -1) {
			AfxMessageBox("连接失败!");
			TRACE("连接失败 : %d %s\r\n", WSAGetLastError(), GetErrorinfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;

	}
	
#define BUFFER_SIZE 4096
	int DealCommand() // 解析命令 from client 
	{
		if (m_sock == -1) return -1;
		//char buffer[1024] = "";
		//缓冲区封装包
		char* buffer = m_buffer.data();
		//TODO: 为什么不用delete buffer ? 服务器收命令 短链接 可能会发送多个数据 收是单个 发是多个
		//为什么需要成员变量处理缓冲区拿包? 服务端会应答多个包 不清楚服务端发送模式 比如TCP每次发包都会应答,那么缓冲区可能有很多个包,我们读取一个包后面就会丢掉
		if (buffer == NULL) {
			TRACE("内存不足! server buffer !\r\n");
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
			//TODO:处理命令
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
	bool GetFilePath(std::string& strPath) {//获取文件路径(列表)
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) { //scmd是俩个字节
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
	CPacket& GetPacket() {//拿到cmd
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
	CClientSocket& operator=(const CClientSocket& ss) {} //ss = server_scoket 重载
	CClientSocket(const CClientSocket& ss)
	{
		m_sock = ss.m_sock;
	}
	CClientSocket() //析构函数
	{
		//m_sock = INVALID_SOCKET;//-1
		if (InitSocketEnv() == FALSE)
		{
			MessageBox(NULL, _T("套接字初始化失败!"), _T("初始化错误!"), MB_OK | MB_ICONERROR); // MFC
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		//m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CClientSocket() //析构函数
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
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CClientSocket* m_instance; //声明实例

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


