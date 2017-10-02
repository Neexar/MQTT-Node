
// MQTTSend.h : главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "включить stdafx.h до включения этого файла в PCH"
#endif

#include "resource.h"		// основные символы


// CMQTTSendApp:
// О реализации данного класса см. MQTTSend.cpp
//

class CMQTTSendApp : public CWinApp
{
public:
	CMQTTSendApp();

// Переопределение
public:
	virtual BOOL InitInstance();

// Реализация

	DECLARE_MESSAGE_MAP()
};

extern CMQTTSendApp theApp;