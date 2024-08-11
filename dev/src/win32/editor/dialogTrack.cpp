// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogTrack.h"
#include "timeline.h"

/*
Virtual constructor.
*/
DialogTrack* DialogTrack::Create( String name, Uint32 depth, bool isGroup ) const
{
	return new DialogTrack( name, depth, isGroup );
}
