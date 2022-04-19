// control_ws.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "control_ws.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}
/*
@ 为什么要加send重载接口?
@ 为了方便,如果没有send 需要手动转换 
*/

//分区 磁盘 创建磁盘分区信息
int MakeDriverInfo() { //1-->A 2-->B 26-->Z // 查看分区是否存在
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size() > 0) result += ',';
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size()); // 打包用 
    Dump((BYTE*)pack.Data(), pack.Size()); // 看看和想要的数据一样嘛  封装了打包过程
    //CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <stdio.h>
#include <io.h>
#include <list>
typedef struct file_info{
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
}FILEINFO,*PFILEINFO;
//查看指定文件夹信息
int MakeDirectoryInfo()//需要传:命令指定路径信息 
{
    std::string strPath;
    //std::list<FILEINFO> listFileInfos;
    //CServerSocket::getInstance()-> 是一个全局的单例!
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("解析获取文件列表命令失败!!!\n"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) { // 如果路径不为0
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //listFileInfos.push_back(finfo);
        CPacket pack(2,(BYTE*) & finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack); //finfo会不停被改
        OutputDebugString(_T("没有权限访问此目录!!!\n"));
        return -2;
    }
    _finddata_t fdata; //结构体
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {//匹配  匹配不成功
        OutputDebugString(_T("没有找到任何文件!!!\n"));
        return -3;
    }
    do {
        //TODO: 把数据传回给控制端
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; //判断是不是文件夹
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
    } while (!_findnext(hfind, &fdata));
    //TODO:发送信息至控制端  对大量文件的解决: 一个一个发
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

//打开文件
int RunFile()
{
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//打开文件  shell
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

//#pragma warning(disable:4996)
//client下载文件
int DownloadFile()
{
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath); // 拿取路径
    long long data = 0;
    FILE* pFile;
    errno_t err = fopen_s(&pFile,strPath.c_str(), "rb");
    if (err != 0) { //如果打开失败
        CPacket pack(4, (BYTE*)&data, 0);//应答一个空数据 对方读到0个字节
        CServerSocket::getInstance()->Send(pack); //告诉你已经结束了
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);//通过8个字节 拿到文件长度发过去
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = ""; // 缓冲区不要大于1k
        size_t rlen = 0;//read len
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack); //读1k 发1k 每个包都能发过去
        } while (rlen >= 1024); //读到文件尾了 x
        fclose(pFile);
    }
    CPacket pack(4, NULL, 0);// 对方收到空的就知道是结尾了
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            // 实现单例
            //CServerSocket* pserver = CServerSocket::getInstance();
            //int count = 0;
            //if (pserver->InitSocket() == false)
            //{
            //        MessageBox(NULL, _T("网络初始化异常!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
            //        exit(0);
            //}
            //while (CServerSocket::getInstance() != NULL)
            //{
            //    if (pserver->AcceptClient() == false)
            //    {
            //        if (count >= 3)
            //        {
            //            MessageBox(NULL, _T("接入用户失败,结束程序!"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("接入用户失败,正在重试!"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO:
            //}
            int nCmd = 1;
            switch (nCmd) {
            case 1: // 查看磁盘分区
                MakeDriverInfo();//拿到磁盘分区信息
                break;
            case 2: // 查看指定目录下文件
                MakeDirectoryInfo();
                break;
            case 3: // 打开文件
                RunFile();
                break; 
            case 4: //下载文件
                DownloadFile();
                break;
            }

        }
                
        
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
