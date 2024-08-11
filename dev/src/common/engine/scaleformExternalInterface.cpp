/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "guiGlobals.h"

#include "scaleformExternalInterface.h"

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// CScaleformExternalInterface
//////////////////////////////////////////////////////////////////////////
void CScaleformExternalInterface::SetHandler( IScaleformExternalInterfaceHandler* handler )
{
	m_handler = handler;
}

void CScaleformExternalInterface::Callback( GFx::Movie* movie, const SFChar* methodName, const GFx::Value* args, SFUInt argCount )
{
	if ( m_handler )
	{
		m_handler->OnExternalInterface( movie, methodName, args, argCount );
	}
	else
	{
		GUI_WARN( TXT("Unhandled ExternalInterface callback '%ls'"), FLASH_UTF8_TO_TXT(methodName) );
	}
}

#endif // USE_SCALEFORM
