/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "guiGlobals.h"

#include "scaleformUserEvent.h"

#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
// CScaleformUserEvent
//////////////////////////////////////////////////////////////////////////
void CScaleformUserEvent::SetHandler( IScaleformUserEventHandler* handler )
{
	m_handler = handler;
}

void CScaleformUserEvent::HandleEvent( GFx::Movie* movie, const GFx::Event& event )
{
	if ( m_handler )
	{
		m_handler->OnUserEvent( movie, event );
	}
}

#endif // USE_SCALEFORM
