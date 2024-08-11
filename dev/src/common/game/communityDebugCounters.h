/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/debugCounter.h"

#ifndef NO_DEBUG_PAGES

///////////////////////////////////////////////////////////////////////////////
class CCommunityMainAgentsCounter : public IDebugCounter
{
private:
	static const Float		AMOUNT_LOW;
	static const Float		AMOUNT_MODERATE;
	static const Float		AMOUNT_HIGH;

public:
	CCommunityMainAgentsCounter();

	//! Get the color for drawing the bar
	virtual Color GetColor() const;

	// Get the text to display
	virtual String GetText() const;

	RED_INLINE void Set( Uint32 n )											{ m_current = Float( n ); }
};
///////////////////////////////////////////////////////////////////////////////

#endif
