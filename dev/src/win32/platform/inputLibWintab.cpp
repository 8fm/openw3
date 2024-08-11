/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "inputLibWintab.h"

static const Char* WINTAB_DLL_NAME = TXT("wintab32.dll");
static HMODULE GWintab;

namespace LibWintab
{
	TFuncInfo				Info;
	TFuncOpen				Open;
	TFuncClose				Close;
	TFuncQueueSizeSet		QueueSizeSet;
	TFuncGet				Get;
	TFuncPacketsGet			PacketsGet;
	TFuncOverlap			Overlap;
}

#define WINTAB_PROC(f) \
	do{\
		LibWintab::f = (LibWintab::TFunc##f)::GetProcAddress( GWintab, "WT" RED_STRINGIFY(f) );\
		if ( !LibWintab::f )\
		{\
			ERR_ENGINE( TXT("%s - GetProcAddress failed for '%s'"), WINTAB_DLL_NAME, "WT" RED_STRINGIFY(f) );\
			void SShutdownLibWintab();\
			SShutdownLibWintab();\
			return false;\
		}\
	}while(false)

Bool SInitLibWintab()
{
	RED_ASSERT( ::SIsMainThread() );

	if ( GWintab )
	{
		return true;
	}

	GWintab = ::LoadLibrary( WINTAB_DLL_NAME );
	if ( ! GWintab )
	{
		LOG_ENGINE( TXT("Wintab for Windows '%s' not found"), WINTAB_DLL_NAME );
		return false;
	}

	WINTAB_PROC( Info );
	WINTAB_PROC( Open );
	WINTAB_PROC( Close );
	WINTAB_PROC( QueueSizeSet );
	WINTAB_PROC( Get );
	WINTAB_PROC( PacketsGet );
	WINTAB_PROC( Overlap );

	if ( ! LibWintab::Info( 0, 0, nullptr ) )
	{
		WARN_ENGINE( TXT("WinTab Services not available") );
		void SShutdownLibWintab();
		SShutdownLibWintab();
		return false;
	}

	return true;
}

void SShutdownLibWintab()
{
	RED_ASSERT( ::SIsMainThread() );

	if ( GWintab )
	{
		::FreeLibrary( GWintab );
		GWintab = nullptr;
	}

	LibWintab::Info = nullptr;
	LibWintab::Open = nullptr;
	LibWintab::Close = nullptr;
	LibWintab::QueueSizeSet = nullptr;
	LibWintab::PacketsGet = nullptr;
	LibWintab::Overlap = nullptr;
};