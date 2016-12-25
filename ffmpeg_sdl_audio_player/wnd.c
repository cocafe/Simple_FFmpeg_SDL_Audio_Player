#include "stdafx.h"
#include "player.h"
#include "wnd.h"

#define TOOLBAR_BTN(IDM, STR) { I_IMAGENONE, IDM, TBSTATE_ENABLED, BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L##STR }
#define TOOLBAR_SEP { I_IMAGENONE, 0, 0, BTNS_SEP, { 0 }, 0, (INT_PTR)NULL }

WndData *gWndData;

static WCHAR *PlaybackStatus[] = {
	L"Playing",
	L"Paused",
	L"Stopped",
	L"Done",
};

TBBUTTON MainToolbarBtns[] = {
	TOOLBAR_BTN(IDM_BTN_OPENFILE,	"OPEN"),

	TOOLBAR_SEP,

	TOOLBAR_BTN(IDM_BTN_STOP,		"STOP"),
	TOOLBAR_BTN(IDM_BTN_PLAY,		"PLAY"),
	TOOLBAR_BTN(IDM_BTN_PAUSE,		"PAUSE"),

	TOOLBAR_SEP,

	TOOLBAR_BTN(IDM_BTN_SEEKPREV,	"BACKWARD"),
	TOOLBAR_BTN(IDM_BTN_SEEKNEXT,	"FORWARD"),

	TOOLBAR_SEP,

	TOOLBAR_BTN(IDM_BTN_VOLUMEDEC,  "VOL-"),
	TOOLBAR_BTN(IDM_BTN_VOLUMEINC,  "VOL+"),
	TOOLBAR_BTN(IDM_BTN_VOLUMEMUTE, "Mute"),

	TOOLBAR_SEP,
};

/**
* Main Window Initialize
*/

int InitWndData(WndData *d, HINSTANCE hInstance)
{
	if (!d)
		return -EINVAL;

	if (!hInstance)
		return -EINVAL;

	ZeroMemory(d, sizeof(WndData));

	d->hInstance = hInstance;

	LoadString(hInstance, IDS_MAIN_TITLE, d->szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDS_WND_CLASS, d->szWndClass, MAX_LOADSTRING);

	d->hMutexMainWndControl = CreateMutex(NULL, FALSE, NULL);
	d->hMutexUpdateStatbar = CreateMutex(NULL, FALSE, NULL);
	d->hSemUpdateStatbar = CreateSemaphore(NULL, 0, 1, NULL);

	return 0;
}

int DeinitWndData(WndData *d)
{
	CloseHandle(d->hMutexMainWndControl);
	CloseHandle(d->hMutexUpdateStatbar);
	CloseHandle(d->hSemUpdateStatbar);

	return 0;
}

WndData *AllocWndData(void)
{
	WndData *data;

	data = (WndData *)calloc(1, sizeof(WndData));
	if (!data)
		return NULL;

	return data;
}

void FreeWndData(WndData **d)
{
	if (!*d)
		return;

	free(*d);
	*d = NULL;
}

int RegisterWndData(WndData *WndData)
{
	if (!WndData)
		return -EINVAL;

	if (gWndData != NULL)
		return -EEXIST;

	gWndData = WndData;

	return 0;
}

int RegisterWndClassEx(WndData *WndData)
{
	if (!WndData)
		return -EINVAL;

	WndData->wcex.cbSize = sizeof(WNDCLASSEX);
	WndData->wcex.style = CS_HREDRAW | CS_VREDRAW;
	WndData->wcex.lpfnWndProc = WndProc;
	WndData->wcex.cbClsExtra = 0;
	WndData->wcex.cbWndExtra = 0;
	WndData->wcex.hInstance = WndData->hInstance;
	WndData->wcex.hIcon = LoadIcon(WndData->hInstance, MAKEINTRESOURCE(IDI_MAINICON32));
	WndData->wcex.hIconSm = LoadIcon(WndData->hInstance, MAKEINTRESOURCE(IDI_MAINICON32));
	WndData->wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndData->wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndData->wcex.lpszMenuName = NULL;
	WndData->wcex.lpszClassName = WndData->szWndClass;

	/* returns FALSE on failure */
	if (!RegisterClassEx(&WndData->wcex))
		return -ENODATA;

	return 0;
}

int CreateMainWnd(WndData *WndData)
{
	if (!WndData)
		return -EINVAL;

	WndData->hMainWnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		WndData->szWndClass,
		WndData->szTitle,
		WS_OVERLAPPEDWINDOW /*^ WS_SIZEBOX*/,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		WndData->hInstance,
		NULL);

	if (!WndData->hMainWnd)
		return -ENODATA;

	return 0;
}

/**
* Main Window Control Initialize
*/

HWND CreateMainStatusBar(WndData *WndData)
{
	HWND hStatus;

	if (!WndData)
		return NULL;

	hStatus = CreateWindowEx(
		0,
		STATUSCLASSNAME,
		NULL,
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0,
		0,
		0,
		0,
		WndData->hMainWnd,
		(HMENU)IDC_MAIN_STATUSBAR,
		GetModuleHandle(NULL),
		NULL);

	if (!hStatus)
		return NULL;

	WndData->hStatbarThr = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)UpdateMainStatusbarThread,
		NULL,
		0,
		&WndData->StatbarThrID
	);

	if (WndData->hStatbarThr == NULL) {
		MessageBoxErr(L"Status bar thread creation failure");
	}

	return hStatus;
}

HWND CreateMainToolbar(WndData *WndData)
{
	HWND hWndToolbar;

	if (!WndData)
		return NULL;

	hWndToolbar = CreateWindowEx(
		0,
		TOOLBARCLASSNAME,
		NULL,
		WS_CHILD | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_AUTOSIZE,
		0,
		0,
		0,
		0,
		WndData->hMainWnd,
		NULL,
		WndData->hInstance,
		NULL);

	if (!hWndToolbar)
		return NULL;

	SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(hWndToolbar, TB_ADDBUTTONS, ARRAY_SIZE(MainToolbarBtns), (LPARAM)&MainToolbarBtns);
	SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
	//SendMessage(hWndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(48, 48));

	ShowWindow(hWndToolbar, TRUE);

	return hWndToolbar;
}

int CreateMainWndControl(WndData *WndData)
{
	if (!WndData)
		return -EINVAL;

	WndData->hToolbar = CreateMainToolbar(WndData);
	if (!WndData->hToolbar)
		return -ENODATA;

	WndData->hStatbar = CreateMainStatusBar(WndData);
	if (!WndData->hStatbar)
		return -ENODATA;

	return 0;
}

/**
* Main Window Refinement
*/

void CenterWnd(HWND hWnd)
{
	SetWindowPos(
		hWnd,
		NULL,
		(GetSystemMetrics(SM_CXFULLSCREEN) - (886 - 388 + 1)) / 2,
		(GetSystemMetrics(SM_CYFULLSCREEN) - (523 - 432 + 1)) / 2,
		/* FIXME */
		(886 - 388),
		(523 - 432),
		/*SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER*/0);
}

int DisplayMainWnd(WndData *WndData)
{
	if (!WndData)
		return -EINVAL;

	CenterWnd(WndData->hMainWnd);
	ShowWindow(WndData->hMainWnd, SW_SHOW);
	UpdateWindow(WndData->hMainWnd);

	return 0;
}

#ifdef DEBUG
void PrintWndRect(HWND hWnd)
{
	RECT Rect;

	if (!GetWindowRect(hWnd, &Rect))
		return;

	pr_console("WndRect t: %d d: %d l: %d r: %d\n", Rect.top, Rect.bottom, Rect.left, Rect.right);
}
#else
void PrintWndRect(HWND hWnd)
{

}
#endif

/**
* Control Event Handling
*/
int OpenAudioFile(HWND hWnd)
{
	WCHAR			wFilePath[MAX_PATH];
	CHAR			aFilePath[MAX_PATH];
	WCHAR			buf[MAX_PATH + MAX_LOADSTRING];
	OPENFILENAME	ofn;

	ZeroMemory(wFilePath, sizeof(wFilePath));
	ZeroMemory(aFilePath, sizeof(aFilePath));
	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"Audio Files\0*.*";
	ofn.lpstrFile = wFilePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Open...";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (FAILED(GetOpenFileName(&ofn))) {
		return -EACCES;
	}

	if (!lstrcmpW(wFilePath, L"")) {
		return -ENODATA;
	}

	/* convert Windows WCHAR to CP_ACP CHAR */
	WideCharToMultiByte(CP_ACP, 0, wFilePath, -1, aFilePath, MAX_PATH, NULL, NULL);

	LoadString(NULL, IDS_MAIN_TITLE, buf, MAX_PATH + MAX_LOADSTRING);

	SwitchPlaybackState(gPlayerIns, PLAYBACK_STOP);
	DeinitPlayerAudio(gPlayerIns);
	if (InitPlayerAudio(
		gPlayerIns,
		aFilePath,
		/* TODO */
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_RATE_44_1KHZ,
		AV_SAMPLE_FMT_S16)) {
		MessageBoxErr(L"Error occurred during opening file\n");
		return -ENODATA;
	}

	wsprintf(buf, L"%s - %s", buf, wFilePath);

	SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)buf);

	return 0;
}

/**
* Window Event Procedure
*/

void NotifyUpdateMainStatusBar(void)
{
	ReleaseSemaphore(gWndData->hSemUpdateStatbar, 1, NULL);
}

int UpdateMainStatusBar(HWND hMainWnd)
{
	HWND hStatus;
	WCHAR buf[MAX_PATH];

	WaitForSingleObject(gWndData->hMutexUpdateStatbar, 0);

	hStatus = GetDlgItem(hMainWnd, IDC_MAIN_STATUSBAR);

	if (!hStatus)
		return -ENODEV;

	/* PLAYBACK STATUS | CURRENT POS | MAX POS | VOL % | */

	if (!gPlayerIns)
		return -ENODEV;

	if (!gPlayerIns->fileload) {
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)L"");
		return 0;
	}

	wsprintf(buf, L"%s | ", PlaybackStatus[GetPlaybackState(gPlayerIns)]);
	wsprintf(buf, L"%s%ldKbps | ", buf, GetAudiofileBitrate(gPlayerIns) / 1000);
	wsprintf(buf, L"%s%dHz | ", buf, GetAudiofileSampleRate(gPlayerIns));
	wsprintf(buf, L"%s%ld / %ld | ", buf, GetPlayerCurrentTimestamp(gPlayerIns), GtePlayerStreamDuration(gPlayerIns));
	wsprintf(buf, L"%sVol: %d%%%s | ", buf, (GetPlayerVolume(gPlayerIns) * 100 / AUDIO_VOLUME_MAX), GetPlayerVolumeMute(gPlayerIns) ? L" (muted)" : L"");

	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)buf);

	ReleaseMutex(gWndData->hMutexUpdateStatbar);

	return 0;
}

DWORD WINAPI UpdateMainStatusbarThread(void)
{
	while (1) {
		WaitForSingleObject(gWndData->hSemUpdateStatbar, INFINITE);
		UpdateMainStatusBar(gWndData->hMainWnd);
	}

	ExitThread(EXIT_SUCCESS);
}

int ProcessWMEvent(HWND hWnd, WORD wID, WORD wEvent)
{
	UNREFERENCED_PARAMETER(wEvent);

	if (WaitForSingleObject(gWndData->hMutexMainWndControl, 0) != WAIT_OBJECT_0) {
		pr_console("%s: window control is busy\n", __func__);
		return -EBUSY;
	}

	switch (wID)
	{
		case IDM_BTN_OPENFILE:
			OpenAudioFile(hWnd);
			break;

		case IDM_BTN_STOP:
			SwitchPlaybackState(gPlayerIns, PLAYBACK_STOP);
			break;

		case IDM_BTN_PLAY:
			SwitchPlaybackState(gPlayerIns, PLAYBACK_PLAY);
			break;

		case IDM_BTN_PAUSE:
			SwitchPlaybackState(gPlayerIns, PLAYBACK_PAUSE);
			break;

		case IDM_BTN_VOLUMEINC:
			SetPlayerVolumeByStep(gPlayerIns, 1);
			break;

		case IDM_BTN_VOLUMEDEC:
			SetPlayerVolumeByStep(gPlayerIns, -1);
			break;

		case IDC_BTN_VOLUMEMUTE:
			TogglePlayerVolumeMute(gPlayerIns);
			break;

		case IDM_BTN_SEEKPREV:
			SeekPlayerDirection(gPlayerIns, SEEK_BACKWARD);
			break;

		case IDM_BTN_SEEKNEXT:
			SeekPlayerDirection(gPlayerIns, SEEK_FORWARD);
			break;

		case IDM_BTN_SEETINGS:
			MessageBoxInfo(L"WIP");
			break;

		default:
			break;
	}

	ReleaseMutex(gWndData->hMutexMainWndControl);

	NotifyUpdateMainStatusBar();

	return 0;
}

void RefineWindowControl(HWND hWnd)
{
	HWND hTool;
	RECT rcTool;
	INT iToolHeight;

	HWND hStatus;
	RECT rcStatus;
	INT iStatusHeight;

	hTool = GetDlgItem(hWnd, IDC_MAIN_TOOLBAR);
	SendMessage(hTool, TB_AUTOSIZE, 0, 0);

	GetWindowRect(hWnd, &rcTool);
	iToolHeight = rcTool.bottom - rcTool.top;

	hStatus = GetDlgItem(hWnd, IDC_MAIN_STATUSBAR);
	SendMessage(hStatus, WM_SIZE, 0, 0);

	GetWindowRect(hStatus, &rcStatus);
	iStatusHeight = rcStatus.bottom - rcStatus.top;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_COMMAND:
			ProcessWMEvent(hWnd, LOWORD(wParam), HIWORD(wParam));
			return TRUE;

		case WM_SIZE:
			PrintWndRect(hWnd);
			RefineWindowControl(hWnd);

			return TRUE;

		case WM_DESTROY:
			TerminateThread(gWndData->hStatbarThr, -EINTR);

			SwitchPlaybackState(gPlayerIns, PLAYBACK_STOP);
			DeinitPlayerAudio(gPlayerIns);
			DeinitPlayerData(gPlayerIns);
			FreePlayerData(&gPlayerIns);

			DeinitWndData(gWndData);
			FreeWndData(&gWndData);

			DeinitDebugConsole();

			PostQuitMessage(EXIT_SUCCESS);

			break;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return FALSE;
}
