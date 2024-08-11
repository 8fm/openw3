/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/
#include "build.h"

void SFrameTracker::SetFrameIndex( Int32 frameIndex )
{
	m_updateFrameIndex.SetValue( frameIndex );
}

const EFrameUpdateState SFrameTracker::UpdateOncePerFrame( Int32 frameIndex )
{
	// Do not update LOD more than once pre frame
	const Int32 lastUpdateFrame = m_updateFrameIndex.Exchange( frameIndex );
	if ( lastUpdateFrame == frameIndex )
	{
		return FUS_AlreadyUpdated;
	}

	// Were we updated last frame also ?
	const Bool wasUpdatedLastFrame = (lastUpdateFrame == (frameIndex - 1));
	return wasUpdatedLastFrame ? FUS_UpdatedLastFrame : FUS_NotUpdatedLastFrame;
}
