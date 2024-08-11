/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <libsysmodule.h>
#include "contentManagerOrbis.h"

//////////////////////////////////////////////////////////////////////////
// CContentManagerOrbis
//////////////////////////////////////////////////////////////////////////
CContentManagerOrbis::CContentManagerOrbis()
{
}

CContentManagerOrbis::~CContentManagerOrbis()
{
	Shutdown();
}

Bool CContentManagerOrbis::Init()
{
	if ( ! InitAppContentLib() )
	{
		return false;
	}

	if ( !TBaseClass::Init() )
	{
		return false;
	}

	return true;
}

Bool CContentManagerOrbis::InitAppContentLib()
{
	Int32 err = ::sceSysmoduleLoadModule( SCE_SYSMODULE_APP_CONTENT );
	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("Failed to load SCE_SYSMODULE_APP_CONTENT: 0x%08X"), err );
		return false;
	}

	SceAppContentInitParam initParam;
	SceAppContentBootParam bootParam;
	Red::System::MemoryZero( &initParam, sizeof(SceAppContentInitParam) );
	Red::System::MemoryZero( &bootParam, sizeof(SceAppContentBootParam) );

	err = ::sceAppContentInitialize( &initParam, &bootParam );
	if (err != SCE_OK )
	{
		ERR_ENGINE(TXT("sceAppContentInitialize failed: 0x%08X"), err );
		return false;
	}

	return true;
}

void CContentManagerOrbis::Shutdown()
{
	Int32 err = ::sceSysmoduleUnloadModule( SCE_SYSMODULE_APP_CONTENT );
	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("Failed to load SCE_SYSMODULE_APP_CONTENT: 0x%08X"), err );
		return;
	}
}
