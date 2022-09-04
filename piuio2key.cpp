// Win32 Dialog.cpp : Defines the entry point for the application.
//


#include "stdafx.h"
#include "piuio2key.h"
#include "PIUIO.h"

using namespace std;

#define TRAYICONID	1//				ID number for the Notify Icon
#define SWM_TRAYMSG	WM_APP//		the message ID sent to our window

#define SWM_ABOUT	WM_APP + 1//	show the window
#define SWM_EXIT	WM_APP + 3//	close the window
#define SWM_DIAG	WM_APP + 5//	close the window

// 1p, 2p, COIN1, COIN2, CONFIG, SERVICE
unsigned nbits[] = {0, 1, 2, 3, 4, 
					16, 17, 18, 19, 20, 
					10, 26, 9, 14};
int done = 0;
WORD wkeys[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 
			0x0, 0x0, 0x0, 0x0, 0x0, 
			0x0, 0x0, 0x0, 0x0};
WORD wscan[] = { 0x51, 0x45, 0x53, 0x5A, 0x43,
				VK_NUMPAD7, VK_NUMPAD9, VK_NUMPAD5, VK_NUMPAD1, VK_NUMPAD3,
				VK_F4, VK_ESCAPE, VK_F1, VK_F2 };
int nIDHigh[11][4] = { 
	{ IDC_CUSTOM1, IDC_CUSTOM2, IDC_CUSTOM3, IDC_CUSTOM4 },
	{ IDC_CUSTOM5, IDC_CUSTOM8, IDC_CUSTOM6, IDC_CUSTOM7 },
	{ IDC_CUSTOM9, IDC_CUSTOM12, IDC_CUSTOM10, IDC_CUSTOM11 },
	{ IDC_CUSTOM13, IDC_CUSTOM16, IDC_CUSTOM14, IDC_CUSTOM15 },
	{ IDC_CUSTOM17, IDC_CUSTOM20, IDC_CUSTOM18, IDC_CUSTOM19 },
	{ IDC_CUSTOM21, IDC_CUSTOM24, IDC_CUSTOM22, IDC_CUSTOM23 },
	{ IDC_CUSTOM25, IDC_CUSTOM28, IDC_CUSTOM26, IDC_CUSTOM27 },
	{ IDC_CUSTOM29, IDC_CUSTOM32, IDC_CUSTOM30, IDC_CUSTOM31 },
	{ IDC_CUSTOM33, IDC_CUSTOM36, IDC_CUSTOM34, IDC_CUSTOM35 },
	{ IDC_CUSTOM37, IDC_CUSTOM40, IDC_CUSTOM38, IDC_CUSTOM39 },
	{ IDC_CUSTOM41, IDC_CUSTOM42, IDC_CUSTOM43, IDC_CUSTOM44 }
};
int siz = sizeof(nbits) / sizeof(int);

// Global Variables:
HINSTANCE		hInst;	// current instance
NOTIFYICONDATA	niData;	// notify icon data

// Forward declarations of functions included in this code module:
BOOL				InitInstance(HINSTANCE, int);
BOOL				OnInitDialog(HWND hWnd);
void				ShowContextMenu(HWND hWnd);
ULONGLONG			GetDllVersion(LPCTSTR lpszDllName);

INT_PTR CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DlgDiag(HWND, UINT, WPARAM, LPARAM);

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
#ifndef UNICODE  
typedef std::string String;
#else
typedef std::wstring String;
#endif

String GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return String(); //No error message has been recorded

	LPTSTR messageBuffer = nullptr;
	size_t sizez = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&messageBuffer, 0, NULL);

	String message(messageBuffer, sizez);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

template<class T>
inline bool IsBitSet(const T &data, int bit)
{
	// -1, since we have 32 bits indexed as 0..31
	int bits = sizeof(T) * 8 - 1;
	return data & ((T)1 << (bits - bit));
}

HWND hDiag;
int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	char CabLights;

	g_ihPIUIO = new InputHandler_PIUIO();
	if (!g_ihPIUIO->m_bFoundDevice)
	{
		MessageBox((HWND)0, _T("PIUIO not detected\nIf I don't have any PIUIO there is no reason to live!.\nI'm gonna die!. Ciao! BANG!\n\nNah Just kidding, just there is no I/O"), _T("ERROR"), MB_ICONSTOP);
	}

#ifdef PIUIO2KEY_SUPPORT_PIPE
	HANDLE pipe = CreateNamedPipe(
		L"\\\\.\\pipe\\my_pipe", // name of the pipe
		PIPE_ACCESS_INBOUND | PIPE_NOWAIT, // 1-way pipe -- send only (unblocking)
		PIPE_TYPE_BYTE, // send data as a byte stream
		1, // only allow 1 instance of this pipe
		0, // no outbound buffer
		0, // no inbound buffer
		0, // use default wait time
		NULL // use default security attributes
		);

	if (pipe == NULL || pipe == INVALID_HANDLE_VALUE) {
		wcout << "Failed to create outbound pipe instance.";
		// look up error code here using GetLastError()
		return 1;
	}
#endif

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) return FALSE;
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_STEALTHDIALOG);

	HDC hdc = NULL;

	HBRUSH hbrush1 = CreateSolidBrush(RGB(255, 0, 0) );

	HBRUSH hbrush2 = CreateSolidBrush(RGB(0, 0, 0));

	// Main message loop:
	while (!done)
	{
#ifdef PIUIO2KEY_SUPPORT_PIPE
		BOOL result = ConnectNamedPipe(pipe, NULL);
		DWORD dwReason = GetLastError();
		if (!result && dwReason != ERROR_IO_PENDING && dwReason != ERROR_PIPE_CONNECTED) {
			wcout << "Failed to make connection on named pipe." << endl;
			// look up error code here using GetLastError()
			CloseHandle(pipe); // close the pipe
			return 1;
		}
		else if (result)
		{
			DWORD cbRead;
			result = ReadFile(
				pipe,
				&CabLights,
				1,
				&cbRead,
				NULL);
			if (result && cbRead != 0)
			{
				// Put all the data where it belongs
				//g_ihPIUIO->m_iLightData = 0;
				TCHAR msg[260];
				_stprintf(msg, TEXT("State of lights: %.2x.\n"), (uint32_t)CabLights);
				MessageBox(NULL, msg, _T("no error"), MB_ICONASTERISK);
				if ((CabLights >> 0) & 0x1) g_ihPIUIO->m_iLightData |= 1 << 23;
				if ((CabLights >> 1) & 0x1) g_ihPIUIO->m_iLightData |= 1 << 24;
				if ((CabLights >> 2) & 0x1) g_ihPIUIO->m_iLightData |= 1 << 25;
				if ((CabLights >> 3) & 0x1) g_ihPIUIO->m_iLightData |= 1 << 26;
				if ((CabLights >> 4) & 0x1) g_ihPIUIO->m_iLightData |= 1 << 10;
			}

			// Dont care if there is no data
		}
#endif

		// Process the message queue
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) ||
				!IsDialogMessage(msg.hwnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		/*g_ihPIUIO->m_iInputData[0] = 0xFAFAFAFA;
		g_ihPIUIO->m_iInputData[1] = 0xDEADBEEF;
		g_ihPIUIO->m_iInputData[2] = 0x0B00B1E5;
		g_ihPIUIO->m_iInputData[3] = 0x13131313;
		g_ihPIUIO->m_iInputField = 0xDEADBEEF;*/
		if (!g_ihPIUIO->m_bFoundDevice) continue;
		g_ihPIUIO->InputThreadMain();
		//if (g_ihPIUIO->m_iChanged)
		{
			if (IsWindowVisible(hDiag)) {
				Sleep(10);
				//hdc = GetDC(hDiag);
			}
			for (int i = 0; i < siz; i++)
			//for (unsigned iBtn = 0; iBtn < 32; ++iBtn)
			{
				unsigned iBtnA = nbits[i];
				unsigned iBtn = 31 - iBtnA;
				int nEvent = 0;
				// Diagnostics run
				if (IsWindowVisible(hDiag))
				{
					int nIDShow;
					HWND hControl;
					bool isActive;
					RECT rect;
					// Sensors
					if (i < 10)
					{
						for (int j = 0; j < 4; j++)
						{
							nIDShow = nIDHigh[i][j];
							hControl = GetDlgItem(hDiag, nIDShow);
							isActive = IsBitSet(g_ihPIUIO->m_iInputData[j], iBtn) ? true : false;
							hdc = GetDC(hControl);
							//SetBkColor(hdc, isActive ? RGB(255, 0, 0) : RGB(0, 0, 0));
							//hbrush = CreateSolidBrush(isActive ? RGB(255, 0, 0) : RGB(0, 0, 0));
							GetClientRect(hControl, &rect);
							FillRect(hdc, &rect, isActive ? hbrush1 : hbrush2);
							ReleaseDC(hControl, hdc);
						}
					}
					// Misc Buttons
					else
					{
						nIDShow = nIDHigh[10][i - 10];
						hControl = GetDlgItem(hDiag, nIDShow);
						isActive = IsBitSet(g_ihPIUIO->m_iInputField, iBtn) ? true : false;
						hdc = GetDC(hControl);
						//SetBkColor(hdc, isActive ? RGB(255, 0, 0) : RGB(0, 0, 0));
						//hbrush = CreateSolidBrush(isActive ? RGB(255, 0, 0) : RGB(0, 0, 0));
						GetClientRect(hControl, &rect);
						FillRect(hdc, &rect, isActive ? hbrush1 : hbrush2);
						ReleaseDC(hControl, hdc);
					}
					//Sleep(100);
				}
				// End diagnostics

				// if this button's status has changed, report it.
				if ((!IsBitSet(g_ihPIUIO->m_iChanged, iBtn)))
					continue;
				nEvent = IsBitSet(g_ihPIUIO->m_iInputField, iBtn) ? 1 : 0; //1 for down, 0 for up

				INPUT ip;
				// Set up a generic keyboard event.
				ip.type = INPUT_KEYBOARD;
				ip.ki.wScan = MapVirtualKey(wscan[i], MAPVK_VK_TO_VSC);; // hardware scan code for key
				ip.ki.time = GetTickCount();
				ip.ki.dwExtraInfo = 0;
				ip.ki.wVk = wkeys[i]; // virtual-key code

				if (nEvent)
					ip.ki.dwFlags = 0;
				else
					ip.ki.dwFlags = KEYEVENTF_KEYUP;

				if (ip.ki.wScan)
					ip.ki.dwFlags |= KEYEVENTF_SCANCODE;

				SendInput(1, &ip, sizeof(INPUT));
			}
			if (IsWindowVisible(hDiag)) {
				//ReleaseDC(hDiag, hdc);
			}
		}
		g_ihPIUIO->m_iLightData = (g_ihPIUIO->m_iInputField & 0x001F001F) << 2;
	}
	return (int)msg.wParam;
}

//	Initialize the window and tray icon
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	// prepare for XP style controls
	InitCommonControls();

	// store instance handle and create dialog
	hInst = hInstance;
	HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX),
		NULL, (DLGPROC)DlgProc);
	if (!hWnd)
	{
		String ad = GetLastErrorAsString();
		DWORD dw = GetLastError();
		MessageBox((HWND)0, ad.c_str(), _T("ERROR"), MB_ICONSTOP);
		return FALSE;
	}

	hDiag = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOGDIAG), NULL, (DLGPROC)DlgDiag);
	if (!hDiag)
	{
		String ad = GetLastErrorAsString();
		DWORD dw = GetLastError();
		MessageBox((HWND)0, ad.c_str(), _T("ERROR"), MB_ICONSTOP);
		return FALSE;
	}

	// Fill the NOTIFYICONDATA structure and call Shell_NotifyIcon

	// zero the structure - note:	Some Windows funtions require this but
	//								I can't be bothered which ones do and
	//								which ones don't.
	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	// get Shell32 version number and set the size of the structure
	//		note:	the MSDN documentation about this is a little
	//				dubious and I'm not at all sure if the method
	//				bellow is correct
	ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
	if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
		niData.cbSize = sizeof(NOTIFYICONDATA);
	else niData.cbSize = NOTIFYICONDATA_V2_SIZE;

	// the ID number can be anything you choose
	niData.uID = TRAYICONID;

	// state which structure members are valid
	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	// load the icon
	niData.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_STEALTHDLG),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	// the window to send messages to and the message to send
	//		note:	the message value should be in the
	//				range of WM_APP through 0xBFFF
	niData.hWnd = hWnd;
	niData.uCallbackMessage = SWM_TRAYMSG;

	// tooltip message
	lstrcpyn(niData.szTip, _T("The PIUIO2KEY. By: CkDur"), sizeof(niData.szTip) / sizeof(TCHAR));

	Shell_NotifyIcon(NIM_ADD, &niData);

	// free icon handle
	if (niData.hIcon && DestroyIcon(niData.hIcon))
		niData.hIcon = NULL;

	// call ShowWindow here to make the dialog initially visible

	return TRUE;
}

BOOL OnInitDialog(HWND hWnd)
{
	HICON hIcon = (HICON)LoadImage(hInst,
		MAKEINTRESOURCE(IDI_STEALTHDLG),
		IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	return TRUE;
}

// Name says it all
void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if (hMenu)
	{
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_DIAG, _T("Diag"));
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, _T("About"));
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
			pt.x, pt.y, 0, hWnd, NULL);
		DestroyMenu(hMenu);
	}
}

// Get dll version number
ULONGLONG GetDllVersion(LPCTSTR lpszDllName)
{
	ULONGLONG ullVersion = 0;
	HINSTANCE hinstDll;
	hinstDll = LoadLibrary(lpszDllName);
	if (hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
		pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
		if (pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;
			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);
			hr = (*pDllGetVersion)(&dvi);
			if (SUCCEEDED(hr))
				ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion, 0, 0);
		}
		FreeLibrary(hinstDll);
	}
	return ullVersion;
}

// Message handler for the app
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case SWM_TRAYMSG:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case SWM_ABOUT:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case SWM_DIAG:
			ShowWindow(hDiag, SW_RESTORE);
			break;
		case IDOK:
			ShowWindow(hWnd, SW_HIDE);
			break;
		case SWM_EXIT:
			done = 1;
			DestroyWindow(hWnd);
			//ShowWindow(hWnd, SW_HIDE);
			break;
		}
		return 1;
	case WM_INITDIALOG:
		return OnInitDialog(hWnd);
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		//DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &niData);
		PostQuitMessage(0);
		break;
	}
	return 0;
}

// Message handler for the app
INT_PTR CALLBACK DlgDiag(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case SWM_DIAG:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case IDOK:
			ShowWindow(hWnd, SW_HIDE);
			break;
		case SWM_EXIT:
			done = 1;
			DestroyWindow(hWnd);
			//ShowWindow(hWnd, SW_HIDE);
			break;
		}
		return 1;
	case WM_INITDIALOG:
		return OnInitDialog(hWnd);
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		//DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &niData);
		PostQuitMessage(0);
		break;
	}
	return 0;
}

