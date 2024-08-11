#include "build.h"
#include "communityDebugCounters.h"


#ifndef NO_DEBUG_PAGES

///////////////////////////////////////////////////////////////////////////////

const Float CCommunityMainAgentsCounter::AMOUNT_LOW = 20.f;
const Float CCommunityMainAgentsCounter::AMOUNT_MODERATE = 40.0f;
const Float CCommunityMainAgentsCounter::AMOUNT_HIGH = 80.0f;

CCommunityMainAgentsCounter::CCommunityMainAgentsCounter()
	: IDebugCounter( TXT("Total quest NPCs"), AMOUNT_HIGH )
{}

Color CCommunityMainAgentsCounter::GetColor() const
{
	if ( m_current < AMOUNT_LOW ) return Color( 0, 128, 0 );
	if ( m_current < AMOUNT_MODERATE ) return Color( 128, 128, 0 );
	return Color( 128, 0, 0 );
}

String CCommunityMainAgentsCounter::GetText() const
{
	return String::Printf( TXT("Total quest NPCs: %.0f"), m_current );
}


///////////////////////////////////////////////////////////////////////////////

#endif
