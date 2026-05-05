#pragma once
#include <windows.h>
#include <string>

//BASIC WILL CHANGE TO SDL

typedef void (*RunCallbackType)();

class WinWindows
{
public:
	WinWindows(HINSTANCE hinstance, unsigned int Width, unsigned int Height);

	MSG msg = { 0 };

	virtual bool InitWindow();
	//virtual void Render() = 0;
	int Run();
	//LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:

	HWND m_hAppWindow;
	HINSTANCE m_hAppInstance;
	unsigned int m_uiClientWidth;
	unsigned int m_uiClientHeight;
	std::wstring m_sAppTitle;
	DWORD m_WindowStyle;

	static WinWindows* GameWindow;
};

extern RunCallbackType RunCallback;