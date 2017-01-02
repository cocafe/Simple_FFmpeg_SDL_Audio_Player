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

void player_threadctx_init(PlayerData *player);
void player_threadctx_deinit(PlayerData *player);

LRESULT WINAPI FFMThread(LPVOID data);
LRESULT WINAPI SDLThread(LPVOID data);
LRESULT PlaybackDaemonThread(LPVOID *data);

int player_thread_create(PlayerData *player);
int player_thread_destroy(PlayerData *player);