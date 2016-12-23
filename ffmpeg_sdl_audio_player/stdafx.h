#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
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