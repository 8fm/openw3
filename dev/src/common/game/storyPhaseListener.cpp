/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storyPhaseListener.h"
#include "communityData.h"

CStoryPhaseListener::CStoryPhaseListener( TDynArray< THandle< CCommunity > >& spawnsets )
	: m_spawnsets( spawnsets )
{
}

CStoryPhaseListener::~CStoryPhaseListener()
{
}
