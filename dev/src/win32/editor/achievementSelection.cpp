/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "achievementSelection.h"

void CEdAchievementSelection::FillChoices()
{	
	if ( !GUserProfileManager )
	{
		return;
	}

	TDynArray< CName > names;

	GUserProfileManager->GetAllAchievements( names );

	Uint32 count = names.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( !names[i].Empty() )
		{
			m_ctrlChoice->AppendString( names[i].AsString().AsChar() );
		}
	}
}