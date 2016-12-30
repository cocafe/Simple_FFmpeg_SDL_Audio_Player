/**
 * Header file for player threads
 *
 *		Author: cocafe <cocafehj@gmail.com>
 */

#pragma once

void player_threadctx_init(PlayerData *player);
void player_threadctx_deinit(PlayerData *player);

LRESULT WINAPI FFMThread(LPVOID data);
LRESULT WINAPI SDLThread(LPVOID data);
LRESULT PlaybackDaemonThread(LPVOID *data);

int player_thread_create(PlayerData *player);
int player_thread_destroy(PlayerData *player);