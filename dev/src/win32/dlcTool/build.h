/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define SERIAL_HACK_ENABLED

// ENABLE HACK FOR PATCHING INI FILES
// THIS IS FOR PATCH 0 ONLY - REMEMBER REMEMBER TO REMOVE THIS
#define INI_HACK_ENABLED

#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#include <wchar.h>

// Unicode
typedef wchar_t Char;

// Crap
#include "strings.h"
#include "files.h"
#include "errors.h"
#include "feedback.h"
#include "utils.h"
#include "dlcImporter.h"
