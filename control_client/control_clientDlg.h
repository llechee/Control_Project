
// control_clientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1)//自定义的消息 发送数据包的消息

// CcontrolclientDlg 对话框
class CcontrolclientDlg : public CDialogEx
{
// 构造
public:
	CcontrolclientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONTROL_CLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	static void threadEntryForDownloadFile(void* arg);
	void threadDownloadFile();
	void LoadFileCurrent();//为了删除文件设计
	void LoadFileList();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	//1:查看磁盘分区
	//2:打开指定目录文件
	//3:打开文件
	//4:下载文件
	//5:鼠标操作
	//6:发送屏幕内容
	//7:锁机
	//8:解锁
	//9:删除文件
	//1981为测试用
	//返回值是命令号
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);

// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;//下载大文件用

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedBtnTest();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnOpenFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
};
