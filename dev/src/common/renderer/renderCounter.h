/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/debugCounter.h"

#ifndef NO_DEBUG_PAGES

/// Debug counter for rendering thread
class IRenderDebugCounter : public IDebugCounter
{
protected:
	static TDynArray< IRenderDebugCounter* >		s_allCounters;

protected:
	Float		m_curValue;

public:
	IRenderDebugCounter( const String& name, Float max );
	~IRenderDebugCounter();

	//! Add float based value
	void AddValue( Float value );

	//! Add integer based value
	void AddValueInt( Int32 value );

	//! Flush value
	void Flush();

public:
	//! Flush all values
	static void FlushAll();
};

#endif