#include "WindowsWindow.h"

RunCallbackType RunCallback = nullptr;
WinWindows* WinWindows::GameWindow = nullptr;

WinWindows::WinWindows(HINSTANCE hinstance, unsigned int Width, unsigned int Height)
{
	m_hAppInstance = hinstance;
	m_hAppWindow = NULL;

	m_uiClientWidth = Width;
	m_uiClientHeight = Height;
	m_sAppTitle = L"Gold_BF_PC";
	m_WindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	DefWindowProc(hwnd, msg, wParam, lParam);
	return 1;
}

bool WinWindows::InitWindow()
{
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.hInstance = m_hAppInstance;
	wcex.lpfnWndProc = MainWindowProc;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"BATTLEFRONTWINDOWCLASS";
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, L"Failed to register window class", NULL, NULL);
		return false;
	}

	RECT r = { 0, 0, m_uiClientWidth, m_uiClientHeight };

	AdjustWindowRect(&r, m_WindowStyle, false);
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	m_hAppWindow = CreateWindow(L"BATTLEFRONTWINDOWCLASS", m_sAppTitle.c_str(), m_WindowStyle, GetSystemMetrics(SM_CXSCREEN) / 2 - width / 2, GetSystemMetrics(SM_CYSCREEN) / 2 - height / 2, width, height, NULL, NULL, m_hAppInstance, NULL);

	if (!m_hAppWindow)
	{
		MessageBox(NULL, L"Failed to create window", NULL, NULL);
		return false;
	}
	ShowWindow(m_hAppWindow, SW_SHOW);


	return true;
}

int WinWindows::Run()
{
	if(WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			/*if (RunCallback)
			{
				RunCallback();
			}*/
		}
		return 1;
	}
	return static_cast<int>(msg.wParam);
}
