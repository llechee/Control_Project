﻿
// control_clientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "control_client.h"
#include "control_clientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CcontrolclientDlg 对话框



CcontrolclientDlg::CcontrolclientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CONTROL_CLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CcontrolclientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

int CcontrolclientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));//TODO:返回值需要处理
	if (!ret) {
		AfxMessageBox("网络初始化错误!");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Client Send ret %d\r\n", ret);//是否发送成功
	int cmd = pClient->DealCommand();
	TRACE("Client ack : %d\r\n", cmd);
	if (bAutoClose) pClient->CloseSocket();//如果是自动关闭的
	return cmd;
}

BEGIN_MESSAGE_MAP(CcontrolclientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CcontrolclientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CcontrolclientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CcontrolclientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CcontrolclientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CcontrolclientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CcontrolclientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CcontrolclientDlg::OnDeleteFile)
	ON_COMMAND(ID_OPEN_FILE, &CcontrolclientDlg::OnOpenFile)
	ON_MESSAGE(WM_SEND_PACKET,&CcontrolclientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CcontrolclientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CcontrolclientDlg 消息处理程序

BOOL CcontrolclientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001;//127.0.0.1
	m_nPort = _T("9537");
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);//隐藏正在下载页面
	m_isFull = false;//初始化 缓冲区为0
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CcontrolclientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CcontrolclientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CcontrolclientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CcontrolclientDlg::OnBnClickedBtnTest()
{
	//UpdateData();//将控件的值赋值给成员变量
	//// TODO: 在此添加控件通知处理程序代码
	//CClientSocket* pClient = CClientSocket::getInstance();
	//bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));//TODO:返回值需要处理
	//if (!ret) {
	//	AfxMessageBox("网络初始化错误!");
	//	return;
	//}
	//CPacket pack(1981, NULL, 0);
	//ret = pClient->Send(pack);
	//TRACE("Client Send ret %d\r\n", ret);//是否发送成功
	//int cmd = pClient->DealCommand();
	//TRACE("Client ack : %d\r\n", cmd);
	////TRACE("Client ack : %d\r\n", pClient->GetPacket().sCmd);
	//pClient->CloseSocket();
	//已简化
	SendCommandPacket(1981);
}


void CcontrolclientDlg::OnBnClickedBtnFileinfo()//获取驱动信息的代码
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败!"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++) {
	//for (size_t i = drivers.size()-1; i > 0; i--) {
		if (drivers[i] == ',') {
			dr = dr + ':';
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr = dr + drivers[i];
	}
	//TODO:需要优化 自定义
	dr = dr + ':';
	HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
	m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
	dr.clear();
}


void CcontrolclientDlg::threadEntryForSendSreen(void* arg)
{
	CcontrolclientDlg* thiz = (CcontrolclientDlg*)arg;
	thiz->threadSendSreen();
	_endthread();
}


void CcontrolclientDlg::threadSendSreen()
{
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();
	} while (pClient == NULL);
	for (;;) {
		CPacket pack(6, NULL, 0);
		bool ret = pClient->Send(pack);
		if (ret) {
			int cmd = pClient->DealCommand();
			if (cmd == 6) {
				if (m_isFull == false) {//更新数据到缓存
					BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();//TODO: 存入cimage
					HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
					if (hMen == NULL) {
						TRACE("about screen 内存不足");
						Sleep(1);
						continue;
					}
					IStream* pStream = NULL;
					HRESULT hRet = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
					if (hRet == S_OK) {
						ULONG length = 0;
						pStream->Write(pData, pClient->GetPacket().strData.size(), &length);
						LARGE_INTEGER bg = { 0 };
						pStream->Seek(bg, STREAM_SEEK_SET, NULL);
						m_image.Load(pStream);
						m_isFull = true;
					}
				}
			}
		}
		else {
			Sleep(1);
		}
	}
}


void CcontrolclientDlg::threadEntryForDownloadFile(void* arg)
{
	CcontrolclientDlg* thiz = (CcontrolclientDlg*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CcontrolclientDlg::threadDownloadFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);//获得点击的标记 
	CFileDialog dlg(FALSE, NULL,
		strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");//拿到完整路径名后打开
		if (pFile == NULL) {
			AfxMessageBox(_T("文件打开异常,没有权限保存文件!"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();//拿路径
		strFile = GetPath(hSelected) + strFile;//拿文件名 拼path
		TRACE("getapth %s\r\n", LPCSTR(strFile));
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
			if (ret < 0) {
				AfxMessageBox("下载文件失败!");
				TRACE("下载失败 : ret = %d\r\n", ret);
				break;
			}
			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0) {//读不到文件或者文件是0字节
				AfxMessageBox("文件无法读取!");
				break;
			}
			long long nCount = 0;
			while (nCount < nLength) {
				ret = pClient->DealCommand();
				if (ret < 0) {
					AfxMessageBox("传输失败!");
					TRACE("文件传输失败 ret = %d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);//每成功接收到一个包,就把size写到文件里面去
				nCount += pClient->GetPacket().strData.size();
			}
		} while (false);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成!"), _T("完成"));
}

CString CcontrolclientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CcontrolclientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());//获取点击信息
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		TRACE("[%s] dir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("tree button ack : %d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	//hastext为空 数据就是无效的
	pClient->CloseSocket();
}

void CcontrolclientDlg::LoadFileList()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);//判断点击
	if (hTreeSelected == NULL) return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) return;
	DeleteTreeChildrenItem(hTreeSelected);//防止双击多次
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());//获取点击信息
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	int Count = 0;//计数用
	while (pInfo->HasNext) {
		TRACE("[%s] dir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == "..")) {
				int cmd = pClient->DealCommand();
				TRACE("deal . ack : %d", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		Count++;
		int cmd = pClient->DealCommand();
		TRACE("tree button ack : %d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	//hastext为空 数据就是无效的
	pClient->CloseSocket();
	TRACE("Count : %d\r\n", Count);
}

void CcontrolclientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL) m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


void CcontrolclientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileList();
}


void CcontrolclientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileList();
}


void CcontrolclientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)//目录右键相关操作
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);//加载菜单资源
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTALIGN, ptMouse.x, ptMouse.y, this);
	}
}


void CcontrolclientDlg::OnDownloadFile()
{
	////添加线程函数
	_beginthread(CcontrolclientDlg::threadEntryForDownloadFile, 0, this);
	// TODO: 在此添加命令处理程序代码
	//Sleep(50);
	BeginWaitCursor();//光标
	m_dlgStatus.m_info.SetWindowText(_T("执行中......"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
}


void CcontrolclientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//拿到选中的节点
	CString strPath = GetPath(hSelected);//拿到路径
	int nSelected = m_List.GetSelectionMark();//拿到列表选中的
	CString strFile = m_List.GetItemText(nSelected, 0);//拿到文件名
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());//发送请求报文
	if (ret < 0) {
		AfxMessageBox("删除文件失败!");
	}
	LoadFileCurrent();
}


void CcontrolclientDlg::OnOpenFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//拿到选中的节点
	CString strPath = GetPath(hSelected);//拿到路径
	int nSelected = m_List.GetSelectionMark();//拿到列表选中的
	CString strFile = m_List.GetItemText(nSelected, 0);//拿到文件名
	strFile = strPath + strFile;
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());//发送请求报文
	if (ret < 0) {
		AfxMessageBox("打开文件失败!");
	}
}

LRESULT CcontrolclientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	CString strFile = (LPCSTR)lParam;
	int ret = SendCommandPacket(wParam >> 1,wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	return ret;
}


void CcontrolclientDlg::OnBnClickedBtnStartWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	_beginthread(CcontrolclientDlg::threadEntryForSendSreen, 0, this);
	CWatchDialog dlg(this);
	dlg.DoModal();
}


void CcontrolclientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}
