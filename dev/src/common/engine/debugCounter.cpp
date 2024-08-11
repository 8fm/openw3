/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugCounter.h"
#include "baseEngine.h"

#ifndef NO_DEBUG_PAGES

IDebugCounter::IDebugCounter( const String& name, Float max )
	: m_name( name )
	, m_max( max )
	, m_current( 0.0f )
{
	if ( GEngine ) 
	{
		GEngine->RegisterDebugCounter( this );
	}
}

IDebugCounter::~IDebugCounter()
{
	if ( GEngine ) 
	{
		GEngine->UnregisterDebugCounter( this );
	}
}

Color IDebugCounter::GetColor() const
{
	const Float prc = m_current / m_max;
	if ( prc > 0.75f ) return Color( 0, 128, 0 );
	if ( prc > 0.5f ) return Color( 128, 128, 0 );
	return Color( 128, 0, 0 );
}

String IDebugCounter::GetText() const
{
	return String::Printf( TXT("%s: %1.1f"), m_name.AsChar(), m_current );
}

#endif