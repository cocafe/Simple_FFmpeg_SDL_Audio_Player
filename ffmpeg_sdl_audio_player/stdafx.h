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

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>

#include <Windows.h>

#include "resource.h"
#include "debug.h"

#define MessageBoxInfo(Msg)		do { MessageBox(						\
										NULL,							\
										Msg,							\
										L"Simple ffmpeg player",		\
										MB_ICONINFORMATION | MB_OK); } while (0)

#define MessageBoxErr(Msg)		do { MessageBox(						\
										NULL,							\
										Msg,							\
										L"Simple ffmpeg player",		\
										MB_ICONERROR | MB_OK); } while (0)

#define ARRAY_SIZE(a)			(SSIZE_T)(sizeof(a) / sizeof(a[0]))