
// MQTTSend.h : ������� ���� ��������� ��� ���������� PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�������� stdafx.h �� ��������� ����� ����� � PCH"
#endif

#include "resource.h"		// �������� �������


// CMQTTSendApp:
// � ���������� ������� ������ ��. MQTTSend.cpp
//

class CMQTTSendApp : public CWinApp
{
public:
	CMQTTSendApp();

// ���������������
public:
	virtual BOOL InitInstance();

// ����������

	DECLARE_MESSAGE_MAP()
};

extern CMQTTSendApp theApp;