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

#include "stdafx.h"
#include "player.h"
#include "wnd.h"

INT APIENTRY wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdline,
	INT nCmdshow
)
{
	PlayerData *player;
	WndData *wndd;

	InitDebugConsole();
	CreatePCMDump(PCM_DUMP_FILEPATH);

	wndd = AllocWndData();
	if (!wndd)
		goto err_wndd_alloc;

	player = AllocPlayerData();
	if (!player)
		goto err_player_alloc;

	if (InitWndData(wndd, hInstance)) {
		MessageBoxErr(L"Initialize window failure");
		goto free_wndd;
	}

	if (InitPlayerData(player)) {
		MessageBoxErr(L"Initialize player failure");
		goto free_player;
	}

	if (RegisterWndData(wndd)) {
		MessageBoxErr(L"Register window failure");
		goto deinit_wndd;
	}

	if (RegisterPlayerData(player)) {
		MessageBoxErr(L"Register player failure");
		goto deinit_player;
	}

	if (RegisterWndClassEx(wndd)) {
		MessageBoxErr(L"Register window class failure");
		goto deinit_wndd;
	}

	if (CreateMainWnd(wndd)) {
		MessageBoxErr(L"Main Window creation failure");
		goto deinit_wndd;
	}

	if (CreateMainWndControl(wndd)) {
		MessageBoxErr(L"Main Window control(s) creation failure");
		goto deinit_wndd;
	}

	DisplayMainWnd(wndd);

	while (GetMessage(&wndd->MsgMainWnd, NULL, 0, 0)) {
		TranslateMessage(&wndd->MsgMainWnd);
		DispatchMessage(&wndd->MsgMainWnd);
	}

	return (INT)wndd->MsgMainWnd.wParam;

deinit_player:
	DeinitPlayerData(player);

deinit_wndd:
	DeinitWndData(wndd);

free_player:
	FreePlayerData(&player);

err_player_alloc:
free_wndd:
	FreeWndData(&wndd);

err_wndd_alloc:
	DeinitDebugConsole();

	return EXIT_SUCCESS;
}