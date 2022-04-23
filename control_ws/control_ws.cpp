// control_ws.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "control_ws.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>

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
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <stdio.h>
#include <io.h>
#include <list>

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

// 鼠标事件 鼠标点击操作
int MouseEvent()
{
    MOUSEEVENT mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {//远程获取鼠标消息
        DWORD nFlags = 0;//标记  
        //***~低四位表示按下的键位  高四位表示鼠标事件~***//
        switch (mouse.nButtom) {
        case 0://左键
            nFlags = 1;//为什么是1 2 4 8 因为在二进制里面分别代表一个比特位
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中键
            nFlags = 4;
            break;
        case 4://没有按键
            nFlags = 8;
            break;
        }
        if(nFlags!=8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); // 鼠标滑动至位置
        switch (mouse.nAction) {
        case 0://单击
            nFlags |= 0x10;
            break;
        case 1://双击
            nFlags |= 0x20;
            break;
        case 2://按下
            nFlags |= 0x40;
            break;
        case 3://放开 松开
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        switch (nFlags)
        {
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());//系统API 获得当前线程中额外的信息(键盘和鼠标)
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11://左键单击
            //模拟鼠标 全局
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());//系统API 获得当前线程中额外的信息(键盘和鼠标)
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单机
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08://鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(5, NULL, 0);
        CServerSocket::getInstance()->Send(pack); // 返回一个包(空的) 告诉tcp我收到并已经执行完成这个事情了
    }
    else {
        OutputDebugString(_T("获取鼠标参数失败!!!\n")); 
        return -1;
    }
    return 0;
}

// 发送屏幕
int SendScreen()
{
    CImage screen;//GDI 全局设备接口
    HDC hScreen = ::GetDC(NULL);//获取设备上下文 屏幕句柄
    // 一个点上有多少个比特 下VVVV
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); //和RGB 像素点相关 一般是32和28
    int nWidth = GetDeviceCaps(hScreen, HORZRES);//拿到宽度
    int nHeight = GetDeviceCaps(hScreen, VERTRES);//拿到长度
    screen.Create(nWidth, nHeight, nBitPerPixel);//按照显示器创建好图像
    BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hScreen, 0, 0, SRCCOPY);//将图像复制到screen里
    ReleaseDC(NULL, hScreen);//清空
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配到一个可变化内存里去
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//让系统去管理流 TODO:
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);//压缩 保存到流里面去
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }
    
    //DWORD tick = GetTickCount64();//得到时间 来观察生成照片需要耗费的资源
    //screen.Save(_T("Test.png"), Gdiplus::ImageFormatPNG);//压缩
    //TRACE("png %d\r\n", GetTickCount64() - tick);
    //tick = GetTickCount64();
    //screen.Save(_T("Test.jpg"), Gdiplus::ImageFormatJPEG);
    //TRACE("jpg %d\r\n", GetTickCount64() - tick);
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();//释放
    return 0;
}

#include "LockDialog.h"
CLockDialog dlg;//非模态
unsigned threadid = 0;

unsigned __stdcall threadLockDlg(void* arg)//线程函数
{
    dlg.Create(IDD_DIALOG_INFO, NULL);//MFC
    dlg.ShowWindow(SW_SHOW);
    //遮蔽后台窗口
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    rect.bottom *= 1.08;
    //TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);
    dlg.MoveWindow(rect);
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//窗口置顶!
    ShowCursor(false);//不显示鼠标
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//隐藏Windows任务栏
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    //dlg.GetWindowRect(rect);//获得界面大小
    ClipCursor(rect);//鼠标限制在范围内
    MSG msg;//需要建立 消息循环
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN) {//按下关闭对话框
            //TRACE("msg:%08X wparam:%08x lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam); // 用来监测按键的进制码 键盘
            if (msg.wParam == 0x41) {//按下ESC退出 esc:0x1B a:0x41
                break;
            }
        }
    }
    ShowCursor(true);
    dlg.DestroyWindow();
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    _endthreadex(0);
    return 0;
}
//锁机
int LockMachine()
{
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {//检查dlg是否初始化
        _beginthreadex(NULL , 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d\r\n", threadid);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

//解锁机
int UnLockMachine()
{
    //dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);//思考 线程消息的传递
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
    CPacket pack(7777, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

//连接测试
int testconnect()
{
    CPacket pack(1981, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret = %d\r\n", ret);
    return 0;
}

int ExcuteCommand(int nCmd)
{
    int ret = 0;
    switch (nCmd) {
    case 1: // 查看磁盘分区
        ret = MakeDriverInfo();//拿到磁盘分区信息
        break;
    case 2: // 查看指定目录下文件
        ret = MakeDirectoryInfo();
        break;
    case 3: // 打开文件
        ret = RunFile();
        break;
    case 4: //下载文件
        ret = DownloadFile();
        break;
    case 5: //鼠标事件
        ret = MouseEvent();
        break;
    case 6://发送屏幕内容==发送屏幕截图
        ret = SendScreen();
        break;
    case 7://锁机  禁止用户操作鼠标键盘菜单等
        ret = LockMachine();
        break;
    case 8://解锁
        ret = UnLockMachine();
        break;
    case 1981:
        ret = testconnect();
        break;
    }
    return ret;
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
            CServerSocket* pserver = CServerSocket::getInstance();
            int count = 0;
            if (pserver->InitSocket() == false)
            {
                    MessageBox(NULL, _T("网络初始化异常!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
                    exit(0);
            }
            while (CServerSocket::getInstance() != NULL)
            {
                if (pserver->AcceptClient() == false)
                {
                    if (count >= 3)
                    {
                        MessageBox(NULL, _T("多次接入用户失败,程序结束!"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("接入用户失败,正在重试!"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
                    count++;
                }
                TRACE("Acceptclirnt return true\r\n");
                int ret = pserver->DealCommand();//获取命令
                TRACE("dealcommand ret : %d\r\n", ret);
                //TODO:
                if (ret > 0) {//如果命令接收ok的话
                    ret = ExcuteCommand(ret);//用ret来表明command
                    if (ret != 0) {
                        TRACE("执行命令失败 : %d ret = %d\r\n", pserver->GetPacket().sCmd, ret);//看看到底是谁的命令执行失败
                    }//需要定义短连接
                    pserver->CloseClient();
                    TRACE("Command has done!\r\n");
                }
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
