/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#define NOWTFUNCTIONS

#include "../../../external/wintab/include/wintab.h"

#undef NOWTFUNCTIONS

namespace LibWintab
{
	typedef UINT (API * TFuncInfo)( UINT, UINT, LPVOID );
	typedef HCTX (API * TFuncOpen)( HWND, LPLOGCONTEXTW, BOOL );
	typedef HCTX (API * TFuncClose)( HCTX );
	typedef BOOL (API * TFuncQueueSizeSet)( HCTX, int );
	typedef BOOL (API * TFuncGet)(HCTX, LPLOGCONTEXT);
	typedef int  (API * TFuncPacketsGet)(HCTX, int, LPVOID);
	typedef BOOL (API * TFuncOverlap)(HCTX, BOOL);

	extern TFuncInfo				Info;
	extern TFuncOpen				Open;
	extern TFuncClose				Close;
	extern TFuncQueueSizeSet		QueueSizeSet;
	extern TFuncGet					Get;
	extern TFuncPacketsGet			PacketsGet;
	extern TFuncOverlap				Overlap;
}
