// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

#include "track.h"

class CEdTimeline;

struct DialogTrack : Track
{
	DialogTrack( String name, Uint32 depth = 0, bool isGroup = false );

	virtual DialogTrack* Create( String name, Uint32 depth, bool isGroup ) const;

	Bool m_isLocked;
};

RED_INLINE DialogTrack::DialogTrack( String name, Uint32 depth /* = 0 */, bool isGroup /* = false */ ) 
: Track( name, depth, isGroup )
, m_isLocked( false )
{}
