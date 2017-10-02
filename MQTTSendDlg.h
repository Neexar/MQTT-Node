
// MQTTSendDlg.h : файл заголовка
//

#pragma once

#include "MQTTClient.h"
#include "afxwin.h"

// диалоговое окно CMQTTSendDlg
class CMQTTSendDlg : public CDialogEx
{
// Создание
public:
	CMQTTSendDlg(CWnd* pParent = NULL);	// стандартный конструктор

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MQTTSEND_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// поддержка DDX/DDV

// Реализация
protected:
	HICON m_hIcon;
	HICON m_hTrayIcon;

	// Созданные функции схемы сообщений
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();	
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnTrayHide();
	afx_msg void OnTrayShow();
	afx_msg void OnTrayExit();	
	afx_msg void OnSize(UINT nType, int cx, int cy);

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL DestroyWindow();

	bool Init();
	int ReadConfig();
	int WriteConfig();
	HRESULT GetCpuTemperature(LPLONG pTemperature);

	CMQTTClient m_Client;
	
	CComboBox m_Combo1;
	CComboBox m_Interval;
	CListBox m_List;
	CEdit m_Address;
	CEdit m_UserName;
	CEdit m_Password;	
	CString m_sDiag;	
	CStatusBar m_bar;

	NOTIFYICONDATA m_trayNID;

	UINT m_trayMessage;
	UINT m_clStatus;

	int m_TimerID;
	int m_cnt;
	int m_retry;
	afx_msg void OnBnClickedCheck2();
};
