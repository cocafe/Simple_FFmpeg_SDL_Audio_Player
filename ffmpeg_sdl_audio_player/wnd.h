/**
 * Copyright 2016 cocafe <cocafehj@gmail.com> All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include "stdafx.h"

#include <Windows.h>
#include <ShObjIdl.h>
#include <CommCtrl.h>

/**
* Toolbar button commands
*/
enum {
	IDM_BTN_OPENFILE = 0,

	IDM_BTN_PLAY,
	IDM_BTN_STOP,
	IDM_BTN_PAUSE,

	IDM_BTN_SEEKPREV,
	IDM_BTN_SEEKNEXT,

	IDM_BTN_VOLUMEINC,
	IDM_BTN_VOLUMEDEC,
	IDM_BTN_VOLUMEMUTE,

	IDM_BTN_SEETINGS,

	NB_TBBTN_IDM
};

/**
* Main window controls
*/
enum {
	IDC_BTN_OPENFILE = 0,

	IDC_BTN_PLAY,
	IDC_BTN_STOP,
	IDC_BTN_PAUSE,

	IDC_BTN_SEEKPREV,
	IDC_BTN_SEEKNEXT,

	IDC_BTN_VOLUMEINC,
	IDC_BTN_VOLUMEDEC,
	IDC_BTN_VOLUMEMUTE,

	IDC_BTN_SEETINGS,

	IDC_MAIN_TOOLBAR,
	IDC_MAIN_STATUSBAR,

	NB_MWND_CONTROL
};

typedef struct WndData {
	HINSTANCE						hInstance;
	WNDCLASSEX						wcex;

	HWND							hMainWnd;
	MSG								MsgMainWnd;
	HANDLE							hMutexMainWndControl;

	HWND							hToolbar;

	HWND							hStatbar;
	DWORD							StatbarThrID;
	HANDLE							hStatbarThr;
	HANDLE							hSemUpdateStatbar;
	HANDLE							hMutexUpdateStatbar;
#define MainStatusBarUpdateInterval	(1000)

	/* TODO: Track bar */

#define MAX_LOADSTRING				(100)
	WCHAR							szTitle[MAX_LOADSTRING];
	WCHAR							szWndClass[MAX_LOADSTRING];
} WndData;

extern WndData *gWndData;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI UpdateMainStatusbarThread(void);

int InitWndData(WndData *d, HINSTANCE hInstance);
int DeinitWndData(WndData *d);

WndData *AllocWndData(void);
void FreeWndData(WndData **d);

int RegisterWndData(WndData *WndData);
int RegisterWndClassEx(WndData *WndData);

int CreateMainWnd(WndData *WndData);
int CreateMainWndControl(WndData *WndData);

int DisplayMainWnd(WndData *WndData);

void NotifyUpdateMainStatusBar(void);
int UpdateMainStatusBar(HWND hMainWnd);