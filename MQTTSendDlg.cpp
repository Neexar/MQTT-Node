
// MQTTSendDlg.cpp : файл реализации
//

#include "stdafx.h"
#include "MQTTSend.h"
#include "MQTTSendDlg.h"
#include "afxdialogex.h"
#include "pdh.h"
#include <Wbemidl.h>

#define FLAG_E_CONNECT	0x0001
#define FLAG_S_CONNECT	0x0100
#define TRAY_ID 10111

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "wbemuuid.lib")

static PDH_HQUERY hQuery;
static PDH_HCOUNTER cpuTotal;
static PDH_HCOUNTER memAval;

FILETIME it, pit, kt, pkt, ut, put;

extern char* test;

static UINT BASED_CODE indicators[] =
{
	ID_INDICATOR_NISH,
	ID_INDICATOR_TIME
};

CMQTTSendDlg::CMQTTSendDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MQTTSEND_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hTrayIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMQTTSendDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_List);
	DDX_Control(pDX, IDC_COMBO1, m_Combo1);
	DDX_Control(pDX, IDC_EDIT1, m_Address);
	DDX_Control(pDX, IDC_COMBO2, m_Interval);
	DDX_Control(pDX, IDC_EDIT3, m_UserName);
	DDX_Control(pDX, IDC_EDIT2, m_Password);
}

BEGIN_MESSAGE_MAP(CMQTTSendDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON2, &CMQTTSendDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CMQTTSendDlg::OnBnClickedButton3)
	ON_WM_TIMER()
	ON_COMMAND(ID_TRAY_HIDE, &CMQTTSendDlg::OnTrayHide)
	ON_COMMAND(ID_TRAY_SHOW, &CMQTTSendDlg::OnTrayShow)
	ON_COMMAND(ID_TRAY_EXIT, &CMQTTSendDlg::OnTrayExit)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CHECK2, &CMQTTSendDlg::OnBnClickedCheck2)
END_MESSAGE_MAP()

BOOL CMQTTSendDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	wchar_t wstr[MAX_STRING];
	wchar_t name[MAX_PATH];
	DWORD resr;

	// Задает значок для этого диалогового окна.  Среда делает это автоматически,
	//  если главное окно приложения не является диалоговым
	SetIcon(m_hIcon, TRUE);			// Крупный значок
	SetIcon(m_hIcon, FALSE);		// Мелкий значок

	// create tray icon
	m_trayMessage = RegisterWindowMessage(L"Tray notification");
	
	m_trayNID.cbSize = sizeof(NOTIFYICONDATA);
	m_trayNID.hWnd = GetSafeHwnd();
	m_trayNID.uID = TRAY_ID;
	m_trayNID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_trayNID.uCallbackMessage = m_trayMessage;
	m_trayNID.hIcon = LoadIcon(NULL, IDI_INFORMATION);
	m_trayNID.uTimeout = 3000;
	m_trayNID.dwInfoFlags = 0;
	m_trayNID.dwState = NIS_SHAREDICON;
	m_trayNID.dwStateMask = NIS_SHAREDICON;
	m_trayNID.uVersion = NOTIFYICON_VERSION;

	wcsncpy_s(m_trayNID.szTip, L"Offline", 64);
	wcsncpy_s(m_trayNID.szInfo, L"Running", 64);
	wcsncpy_s(m_trayNID.szInfoTitle, L"Test", 64);

	Shell_NotifyIcon(NIM_ADD, &m_trayNID);

	// init parameters
	m_Combo1.SetCurSel(0);
	m_Interval.SetCurSel(2);

	m_cnt = 0;
	m_retry = 0;
	m_clStatus = FLAG_E_CONNECT;
	
	m_TimerID = SetTimer(1, 5000, 0);
	
	// init pdh
	PdhOpenQuery(NULL, 0, &hQuery);
	PdhAddEnglishCounter(hQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
	PdhAddEnglishCounter(hQuery, L"\\Memory\\Available MBytes", NULL, &memAval);
	
	PdhCollectQueryData(hQuery);

	//init config
	if(ReadConfig()) WriteConfig();

	MultiByteToWideChar(CP_ACP, 0, m_Client.m_Setting.server, -1, wstr, MAX_STRING);
	m_Address.SetWindowText(wstr);
	MultiByteToWideChar(CP_ACP, 0, m_Client.m_Setting.username, -1, wstr, MAX_STRING);
	m_UserName.SetWindowText(wstr);
	MultiByteToWideChar(CP_ACP, 0, m_Client.m_Setting.password, -1, wstr, MAX_STRING);
	m_Password.SetWindowText(wstr);
	MultiByteToWideChar(CP_ACP, 0, m_Client.m_Setting.interval, -1, wstr, MAX_STRING);

	//Create the status bar
	m_bar.Create(this);
	m_bar.SetIndicators(indicators, 2);

	CRect rect;
	GetClientRect(&rect);
	m_bar.SetPaneInfo(0, ID_INDICATOR_NISH,SBPS_NORMAL, rect.Width() - 100);
	m_bar.SetPaneInfo(1, ID_INDICATOR_TIME, SBPS_STRETCH, 0);

	//This is where we actually draw it on the screen
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, ID_INDICATOR_TIME);

	//Check autorun setting
	if (RegGetValue(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"MQTTLinkNode", RRF_RT_REG_SZ, NULL, (BYTE*)name, &resr) == ERROR_SUCCESS)
	{
		((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(1);
	}

	//init COM
	CoInitialize(NULL);
	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}

void CMQTTSendDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // контекст устройства для рисования

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Выравнивание значка по центру клиентского прямоугольника
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Нарисуйте значок
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

BOOL CMQTTSendDlg::DestroyWindow()
{
	int lnum = m_List.GetCount();

	for (int index = 0; index < lnum; index++)
	{
		m_Client.RemoveTopic(index);
	}

	Shell_NotifyIcon(NIM_DELETE, &m_trayNID);
	
	CoUninitialize();
	
	return CDialogEx::DestroyWindow();
}

HCURSOR CMQTTSendDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool CMQTTSendDlg::Init()
{
	return false;
}

void CMQTTSendDlg::OnBnClickedButton2()
{
	CString bstr, lstr;
	size_t ci;

	char cstr[64];
	int index;
	int lnum;

	m_Combo1.GetWindowText(bstr);
	
	lnum = m_List.GetCount();

	for (index = 0; index < lnum; index++)
	{
		m_List.GetText(index, lstr);
		if (bstr == lstr)
		{
			AfxMessageBox(L"Такой датчик уже есть в списке");
			return;
		}
	}
	
	m_retry = 0;
	
	index = m_List.AddString(bstr);
	wcstombs_s(&ci, cstr, bstr, 64);

	m_Client.AddTopic(index, cstr);
}

void CMQTTSendDlg::OnBnClickedButton3()
{
	int lsel = m_List.GetCurSel();

	if (lsel != CB_ERR)
	{
		m_List.DeleteString(m_List.GetCurSel());
		m_Client.RemoveTopic(lsel);
	}
}

void CMQTTSendDlg::OnTimer(UINT_PTR nIDEvent)
{
	CTime t1;
	CString str = L"";
	PDH_FMT_COUNTERVALUE counterVal;
	
	char nstr[MAX_STRING];
	wchar_t wstr[MAX_STRING];
	
	int pload = 0;
	int pmem = 0;	
	long temperature = 0;
	
	int lnum = m_List.GetCount();

	if (lnum)
	{
		PdhCollectQueryData(hQuery);		
		m_cnt++;
	}
	else m_clStatus = FLAG_E_CONNECT;

	for (int index = 0; index < lnum; index++)
	{
		if (strstr(m_Client.GetTopic(index), "counter") != NULL)
		{
			_itoa_s(m_cnt, nstr, 10);
			_itow_s(m_cnt, wstr, 10);
			str += wstr; str += L" | ";

			if (m_Client.isConnected(index))
			{
				m_clStatus = FLAG_S_CONNECT;
				m_retry = 0;
				m_Client.SendTopic(index, nstr);
			}
			else 
			{
				m_clStatus = FLAG_E_CONNECT;
				if (m_retry < 10) { m_Client.AddTopic(index, "counter"); m_retry++; }
			}
		}

		if (strstr(m_Client.GetTopic(index), "processor") != NULL)
		{			
			if (ERROR_SUCCESS != PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal))
			{
				GetSystemTimes(&it, &kt, &ut);
				pload = ((((kt.dwLowDateTime - pkt.dwLowDateTime) + (ut.dwLowDateTime - put.dwLowDateTime)) - (it.dwLowDateTime - pit.dwLowDateTime)) * 100 / ((kt.dwLowDateTime - pkt.dwLowDateTime) + (ut.dwLowDateTime - put.dwLowDateTime)));
				pit = it; pkt = kt; put = ut;
			}
			else
			{
				pload = (int)counterVal.doubleValue;
			}
				
			_itoa_s(pload, nstr, 10);
			_itow_s(pload, wstr, 10);
			str += wstr; str += L" | ";
			
			if (m_Client.isConnected(index))
			{
				m_clStatus = FLAG_S_CONNECT;
				m_retry = 0;
				m_Client.SendTopic(index, nstr);
			}
			else
			{
				m_clStatus = FLAG_E_CONNECT;
				if (m_retry < 10) { m_Client.AddTopic(index, "processor"); m_retry++; }
			}
		}

		if (strstr(m_Client.GetTopic(index), "temperature") != NULL)
		{
			GetCpuTemperature(&temperature);
			
			_itoa_s(temperature, nstr, 10);
			_itow_s(temperature, wstr, 10);
			str += wstr; str += L" | ";
			
			if (m_Client.isConnected(index))
			{
				m_clStatus = FLAG_S_CONNECT;
				m_retry = 0;
				m_Client.SendTopic(index, nstr);
			}
			else
			{
				m_clStatus = FLAG_E_CONNECT;
				if (m_retry < 10) { m_Client.AddTopic(index, "temperature"); m_retry++; }
			}
		}

		if (strstr(m_Client.GetTopic(index), "memory") != NULL)
		{
			
			MEMORYSTATUSEX memInfo;
			memInfo.dwLength = sizeof(MEMORYSTATUSEX);
			GlobalMemoryStatusEx(&memInfo);
			DWORDLONG AvailPhys = memInfo.ullAvailPhys;

			pmem = (int)(AvailPhys / (1024*1024));
			
			_itoa_s(pmem, nstr, 10);
			_itow_s(pmem, wstr, 10);
			str += wstr; str += L" | ";
			
			if (m_Client.isConnected(index))
			{
				m_clStatus = FLAG_S_CONNECT;
				m_retry = 0;
				m_Client.SendTopic(index, nstr);
			}
			else
			{
				m_clStatus = FLAG_E_CONNECT;
				if (m_retry < 10) { m_Client.AddTopic(index, "memory"); m_retry++; }
			}
		}
		
		if (strstr(m_Client.GetTopic(index), "switch") != NULL)
		{
			_itoa_s(((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck(), nstr, 10);
			_itow_s(m_Client.GetSwitch(), wstr, 10);
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(m_Client.GetSwitch());

			str += wstr; str += L" | ";

			if (m_Client.isConnected(index))
			{
				m_clStatus = FLAG_S_CONNECT;
				m_retry = 0;
				//m_Client.SendTopic(index, nstr);
			}
			else
			{
				m_clStatus = FLAG_E_CONNECT;
				if (m_retry < 10) { m_Client.AddTopic(index, "switch"); m_retry++; }
			}
		}
	}

	t1 = CTime::GetCurrentTime();
	
	if (FLAG_E_CONNECT & m_clStatus)
	{
		str += "offline";
		m_trayNID.hIcon = LoadIcon(NULL, IDI_ERROR);
		wcsncpy_s(m_trayNID.szTip, L"Not connected", 128);
		Shell_NotifyIcon(NIM_MODIFY, &m_trayNID);
	}

	if (FLAG_S_CONNECT & m_clStatus)
	{
		str += "online";
		m_trayNID.hIcon = LoadIcon(NULL, IDI_INFORMATION);
		if (lnum) wcsncpy_s(m_trayNID.szTip, L"Connected", 128);
		else wcsncpy_s(m_trayNID.szTip, L"Offline", 128);
		Shell_NotifyIcon(NIM_MODIFY, &m_trayNID);
	}

	m_bar.SetPaneText(0, str);
	m_bar.SetPaneText(1, t1.Format(L"%H:%M:%S"));

	CDialogEx::OnTimer(nIDEvent);
}

LRESULT CMQTTSendDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == m_trayMessage)
	{
		if (lParam == WM_RBUTTONUP)
		{
			HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));
			HMENU hSubMenu = ::GetSubMenu(hMenu, 0);

			POINT pos;
			GetCursorPos(&pos);

			SetForegroundWindow();
			TrackPopupMenu(hSubMenu, TPM_RIGHTALIGN, pos.x, pos.y, 0, GetSafeHwnd(), NULL);

			DestroyMenu(hMenu);
		}
		else if (lParam == WM_LBUTTONDBLCLK)
		{
			ShowWindow(SW_RESTORE);
			SetForegroundWindow();
		}
	}

	return CDialogEx::WindowProc(message, wParam, lParam);
}

void CMQTTSendDlg::OnTrayHide()
{
	ShowWindow(SW_HIDE);
}

void CMQTTSendDlg::OnTrayShow()
{
	ShowWindow(SW_RESTORE);
	SetForegroundWindow();
}

void CMQTTSendDlg::OnTrayExit()
{
	PostMessage(WM_CLOSE);
}

void CMQTTSendDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	switch (nType)
	{
	case SIZE_MINIMIZED:
		ShowWindow(SW_HIDE);
		break;
	default:
		break;
	}
}

/*********************************************************************
Get temperature
*********************************************************************/
HRESULT CMQTTSendDlg::GetCpuTemperature(LPLONG pTemperature)
{
	if (pTemperature == NULL)
		return E_INVALIDARG;

	HRESULT hr;

	*pTemperature = -1;

	IWbemLocator *pLocator;
	hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator);
	if (SUCCEEDED(hr))
	{
		IWbemServices *pServices;
		BSTR ns = SysAllocString(L"root\\WMI");
		hr = pLocator->ConnectServer(ns, NULL, NULL, NULL, 0, NULL, NULL, &pServices);
		pLocator->Release();
		SysFreeString(ns);
		if (SUCCEEDED(hr))
		{
			BSTR query = SysAllocString(L"SELECT * FROM MSAcpi_ThermalZoneTemperature");
			BSTR wql = SysAllocString(L"WQL");
			IEnumWbemClassObject *pEnum;
			hr = pServices->ExecQuery(wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
			SysFreeString(wql);
			SysFreeString(query);
			pServices->Release();
			if (SUCCEEDED(hr))
			{
				IWbemClassObject *pObject;
				ULONG returned;
				hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &returned);
				pEnum->Release();
				if (SUCCEEDED(hr))
				{
					BSTR temp = SysAllocString(L"CurrentTemperature");
					VARIANT v;
					VariantInit(&v);
					hr = pObject->Get(temp, 0, &v, NULL, NULL);
					pObject->Release();
					SysFreeString(temp);
					if (SUCCEEDED(hr))
					{
						*pTemperature = V_I4(&v) / 10 - 273;
					}
					VariantClear(&v);
					
				}
			}
		}
	}

	return hr;
}

/*********************************************************************
XML setting exchange
*********************************************************************/
inline void TESTHR(HRESULT _hr)
{
	if FAILED(_hr) throw(_hr);
}

int CMQTTSendDlg::ReadConfig()
{
	char tstr[MAX_STRING];
	int bres = 0;
	size_t ci;

	try
	{
		IXMLDOMDocumentPtr docPtr;
		IXMLDOMNodeListPtr nodeList;
		IXMLDOMNode* pXMLNode;

		//init
		TESTHR(CoInitialize(NULL));
		TESTHR(docPtr.CreateInstance("Msxml2.DOMDocument.3.0"));

		// Load a document.
		_variant_t varXml("config.xml");

		VARIANT_BOOL varBool;
		BSTR nodeName;

		TESTHR(docPtr->load(varXml, &varBool));

		if (varBool)
		{
			// Retrieve a list of media elements.
			TESTHR(docPtr->getElementsByTagName(L"ADDRESS", &nodeList));
			TESTHR(nodeList->nextNode(&pXMLNode));
			TESTHR(pXMLNode->get_text(&nodeName));
			wcstombs_s(&ci, tstr, nodeName, 64);
			strcpy_s(m_Client.m_Setting.server, tstr);

			TESTHR(docPtr->getElementsByTagName(L"LOGIN", &nodeList));
			TESTHR(nodeList->nextNode(&pXMLNode));
			TESTHR(pXMLNode->get_text(&nodeName));
			wcstombs_s(&ci, tstr, nodeName, 64);
			strcpy_s(m_Client.m_Setting.username, tstr);

			TESTHR(docPtr->getElementsByTagName(L"PASSWORD", &nodeList));
			TESTHR(nodeList->nextNode(&pXMLNode));
			TESTHR(pXMLNode->get_text(&nodeName));
			wcstombs_s(&ci, tstr, nodeName, 64);
			strcpy_s(m_Client.m_Setting.password, tstr);

			TESTHR(docPtr->getElementsByTagName(L"INTERVAL", &nodeList));
			TESTHR(nodeList->nextNode(&pXMLNode));
			TESTHR(pXMLNode->get_text(&nodeName));
			wcstombs_s(&ci, tstr, nodeName, 64);
			strcpy_s(m_Client.m_Setting.interval, tstr);

			TESTHR(nodeList->reset());
			TESTHR(nodeList->nextNode(&pXMLNode));
		}
	}
	catch (...)
	{
		bres = -1;
	}

	return bres;
}

int CMQTTSendDlg::WriteConfig()
{
	int bres = 0;

	try 
	{
		IXMLDOMDocumentPtr docPtr;
		IXMLDOMElementPtr ElementPtr;
		IXMLDOMElementPtr RootPtr;
		IXMLDOMNode* pnXMLNode;
		IXMLDOMProcessingInstructionPtr piPtr;

		_variant_t varXml("config.xml");
		_variant_t vtObject;

		VARIANT_BOOL varBool;

		TESTHR(CoInitialize(NULL));
		TESTHR(docPtr.CreateInstance("Msxml2.DOMDocument.3.0"));

		TESTHR(docPtr->loadXML(L"<SETTING></SETTING>", &varBool));
		if (varBool)
		{
			TESTHR(docPtr->get_documentElement(&RootPtr));
			TESTHR(docPtr->createProcessingInstruction(L"xml", L" version='1.0' encoding='UTF-8'", &piPtr));

			vtObject.vt = VT_DISPATCH;
			vtObject.pdispVal = RootPtr;
			vtObject.pdispVal->AddRef();

			docPtr->insertBefore(piPtr, vtObject, &pnXMLNode);
			TESTHR(docPtr->createElement(L"ADDRESS", &ElementPtr));
			RootPtr->appendChild(ElementPtr, &pnXMLNode);
			TESTHR(pnXMLNode->put_text(L"m20.cloudmqtt.com:14088"));

			docPtr->insertBefore(piPtr, vtObject, &pnXMLNode);
			TESTHR(docPtr->createElement(L"LOGIN", &ElementPtr));
			RootPtr->appendChild(ElementPtr, &pnXMLNode);
			TESTHR(pnXMLNode->put_text(L"username"));

			docPtr->insertBefore(piPtr, vtObject, &pnXMLNode);
			TESTHR(docPtr->createElement(L"PASSWORD", &ElementPtr));
			RootPtr->appendChild(ElementPtr, &pnXMLNode);
			TESTHR(pnXMLNode->put_text(L"password"));

			docPtr->insertBefore(piPtr, vtObject, &pnXMLNode);
			TESTHR(docPtr->createElement(L"INTERVAL", &ElementPtr));
			RootPtr->appendChild(ElementPtr, &pnXMLNode);
			TESTHR(pnXMLNode->put_text(L"1"));

			TESTHR(docPtr->save(varXml));
		}
	}
	catch (...)
	{
		bres = -1;
	}

	return bres;
}



void CMQTTSendDlg::OnBnClickedCheck2()
{
	HKEY hkey;

	wchar_t name[MAX_PATH];
	DWORD resr;

	GetModuleFileName(NULL, name, MAX_PATH);

	if (((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck())
	{
		RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, 0);
		RegSetValueEx(hkey, L"MQTTLinkNode", NULL, REG_SZ, (byte*)name, wcslen(name) * 2);	
		RegCloseKey(hkey);
	}
	else
	{
		if (RegGetValue(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"MQTTLinkNode", RRF_RT_REG_SZ, NULL, (BYTE*)name, &resr) == ERROR_SUCCESS)
		{
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, &hkey);
			RegDeleteValue(hkey, L"MQTTLinkNode");
			RegCloseKey(hkey);		
		}
	}
}
