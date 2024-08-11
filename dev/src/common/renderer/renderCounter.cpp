/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

TDynArray< IRenderDebugCounter* > IRenderDebugCounter::s_allCounters;

IRenderDebugCounter::IRenderDebugCounter( const String& name, Float max )
	: IDebugCounter( name, max )
	, m_curValue( 0.0f )
{
	s_allCounters.PushBack( this );
}

IRenderDebugCounter::~IRenderDebugCounter()
{
	s_allCounters.Remove( this );
}

void IRenderDebugCounter::AddValue( Float value )
{
	m_curValue += value;
}

void IRenderDebugCounter::AddValueInt( Int32 value )
{
	m_curValue += value;
}

void IRenderDebugCounter::Flush()
{
	// Propagate
	m_current = m_curValue;
	m_curValue = 0.0f;
}

void IRenderDebugCounter::FlushAll()
{
	for ( Uint32 i=0; i<s_allCounters.Size(); i++ )
	{
		IRenderDebugCounter* counter = s_allCounters[i];
		counter->Flush();
	}
}

#endif