
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameInputSelection.h"

///////////////////////////////////////////////////////////////////////////////

void CGameInputSelection::FillChoices()
{	
	if ( !GGame )
	{
		return;
	}

	CInputManager* inputMgr = GGame->GetInputManager();
	if ( !inputMgr )
	{
		return;
	}

	const TDynArray< CName > names = inputMgr->GetGameEventNames();

	Uint32 count = names.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( !names[i].Empty() )
		{
			m_ctrlChoice->AppendString( names[i].AsString().AsChar() );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
