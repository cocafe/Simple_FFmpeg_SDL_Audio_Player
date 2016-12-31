/**
 * Windows Program Main Entry
 *
 *		Author: cocafe <cocafehj@gmail.com>
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