/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "gameTime.h"

/// Interval of time
struct GameTimeInterval
{
	DECLARE_RTTI_STRUCT( GameTimeInterval );

public:
	GameTime	m_begin;
	GameTime	m_end;

public:
	RED_INLINE Bool DoesContainTime( const GameTime &gameTime, Bool ignoreDays = false ) const
	{
		const GameTime &gameTimeToCompare = ignoreDays ? gameTime % GameTime::DAY : gameTime;

		if ( m_begin <= m_end )
		{
			return m_begin <= gameTimeToCompare && gameTimeToCompare < m_end;
		}
		else
		{
			return !(m_end <= gameTimeToCompare && gameTimeToCompare < m_begin);
		}
	}
};

BEGIN_CLASS_RTTI( GameTimeInterval )
	PROPERTY_CUSTOM_EDIT( m_begin, TXT("Begin of interval"), TXT("GameTimePropertyEditor") );
	PROPERTY_CUSTOM_EDIT( m_end, TXT("End of interval"), TXT("GameTimePropertyEditor") );
END_CLASS_RTTI();
