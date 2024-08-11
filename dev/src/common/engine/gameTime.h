/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Time from game
struct GameTime
{
	DECLARE_RTTI_STRUCT( GameTime );

public:
	static const GameTime DAY;
	static const GameTime HOUR;
	static const GameTime MINUTE;

public:
	Int32		m_seconds;		//!< Seconds

public:
	RED_INLINE GameTime()
		: m_seconds( 0 )
	{
	}

	//! Construct from plain seconds count
	RED_INLINE GameTime( Int32 seconds )
		: m_seconds( seconds )
	{
	}

	//! Constructor from days, hours, minutes and seconds
	RED_INLINE GameTime( Int32 days, Int32 hours, Int32 minutes, Int32 seconds )
	{
		m_seconds = 60 * (60 * ((24 * days) + hours) + minutes) + seconds;
	}

	//! Get seconds
	RED_INLINE Int32 GetSeconds() const
	{
		return m_seconds;
	}

	//! Extract seconds from time interval
	RED_INLINE Uint32 Seconds() const 
	{
		return ( m_seconds % MINUTE.GetSeconds() );
	}

	//! Extract full minutes from time interval
	RED_INLINE Uint32 Minutes() const 
	{
		return ( m_seconds % HOUR.GetSeconds() ) / MINUTE.GetSeconds();
	}

	//! Extract full hours from time interval
	RED_INLINE Uint32 Hours() const
	{
		return ( m_seconds % DAY.GetSeconds() ) / HOUR.GetSeconds();
	}

	//! Extract full days from time interval
	RED_INLINE Uint32 Days() const
	{
		return ( m_seconds / DAY.GetSeconds() );
	}

	//! Convert to float
	RED_INLINE Float ToFloat() const
	{
		return (Float) m_seconds;
	}

public:
	//! Assign from seconds
	RED_INLINE GameTime& operator=( const Int32 seconds )
	{
		m_seconds = seconds;
		return *this;
	}

	//! Assign
	RED_INLINE GameTime& operator=( const GameTime &time )
	{
		m_seconds = time.GetSeconds();
		return *this;
	}

	//! Compare
	RED_INLINE Bool operator==( const GameTime &time ) const
	{
		return m_seconds == time.GetSeconds();
	}

	//! Add two time periods
	RED_INLINE GameTime& operator+=( const GameTime &time )
	{
		m_seconds += time.GetSeconds();
		return *this;
	}

	//! Add seconds to time period
	RED_INLINE GameTime& operator+=( const Int32 seconds )
	{
		m_seconds += seconds;
		return *this;
	}

	//! Subtract two time periods
	RED_INLINE GameTime& operator-=( const GameTime &time )
	{
		m_seconds -= time.GetSeconds();
		return *this;
	}

	//! Subtract seconds from time period
	RED_INLINE GameTime& operator-=( const Int32 seconds )
	{
		m_seconds -= seconds;
		return *this;
	}

	//! Add two time periods
	RED_INLINE GameTime operator+( const GameTime &time ) const
	{
		return GameTime( m_seconds + time.GetSeconds() );
	}

	//! Add seconds to time period
	RED_INLINE GameTime operator+( Int32 time ) const
	{
		return GameTime( time + m_seconds );
	}

	//! Subtract two time periods
	RED_INLINE GameTime operator-( const GameTime &time ) const
	{
		return GameTime( m_seconds - time.GetSeconds() );
	}

	//! Subtract seconds from time period
	RED_INLINE GameTime operator-( Int32 time ) const
	{
		return GameTime( time - m_seconds );
	}

	//! Scale time
	RED_INLINE GameTime operator*( Float scale ) const
	{
		return GameTime( (Int32)( m_seconds * scale ) );
	}

	//! Divide time
	RED_INLINE GameTime operator/( Float scale ) const
	{
		return GameTime( (Int32)( m_seconds / scale ) );
	}

	//! Take modulo
	RED_INLINE GameTime operator%( const GameTime& time ) const
	{
		return GameTime( m_seconds % time.GetSeconds() );
	}

	//! Compare
	RED_INLINE Bool operator<( const GameTime &time ) const
	{
		return m_seconds < time.GetSeconds();
	}

	//! Compare
	RED_INLINE Bool operator<( const Int32 time ) const
	{
		return m_seconds < time;
	}

	//! Compare
	RED_INLINE Bool operator<=( const GameTime &time ) const
	{
		return m_seconds <= time.GetSeconds();
	}

	//! Compare
	RED_INLINE Bool operator>( const GameTime &time ) const
	{
		return m_seconds > time.GetSeconds();
	}

	//! Compare
	RED_INLINE Bool operator>( const Int32 time ) const
	{
		return m_seconds > time;
	}

	//! Compare
	RED_INLINE Bool operator>=( const GameTime &time ) const
	{
		return m_seconds >= time.GetSeconds();
	}

	//! Compare by hours, minutes and seconds
	RED_INLINE Bool IsAfter( const GameTime& time ) const
	{
		return	( Hours() > time.Hours() ) ||																			// hours comparison
				( ( Hours() == time.Hours() ) && ( Minutes() > time.Minutes() ) ) ||									// hours equal, minutes comparison
				( ( Hours() == time.Hours() ) && ( Minutes() == time.Minutes() ) && ( Seconds() >= time.Seconds() ) );	// hours and minutes equal, seconds comparison
	}

public:
	//! Convert to string 
	String ToString() const;

	//! Convert to a string in short form
	String ToPreciseString() const;

public:
	//! Serialization operator
	friend IFile& operator<<( IFile& file, GameTime &time )
	{
		file << time.m_seconds;
		return file;
	}
};

BEGIN_CLASS_RTTI( GameTime );
	PROPERTY_EDIT_NAME( m_seconds, TXT("m_seconds"), TXT("Time") );
END_CLASS_RTTI();

// Structure that wraps up game time - used in scripting language to access GameTimePropertyEditor
struct GameTimeWrapper
{
	DECLARE_RTTI_STRUCT( GameTimeWrapper )

	GameTime					m_gameTime;
};

BEGIN_CLASS_RTTI( GameTimeWrapper )
	PROPERTY_CUSTOM_EDIT( m_gameTime,	TXT("Game time"), TXT( "GameTimePropertyEditor" ) )
END_CLASS_RTTI()


// Conversion for GameTime
template<> RED_INLINE String ToString( const GameTime& value )
{
	String output;

	if ( value.Days() > 0 )
	{
		output = String::Printf( TXT( "%02d:%02d:%02d" ), value.Days(), value.Hours(), value.Minutes() );
	}
	else
	{
		output = String::Printf( TXT( "%02d:%02d" ), value.Hours(), value.Minutes() );
	}
	//LOG_ENGINE( TXT("GameTime: seconds %d interpreted as %s"), value.m_seconds, output.AsChar() );
	return output;
}

RED_INLINE Int32 ReadZeroPaddedInt( const String& text )
{
	const Char* str = text.AsChar();
	while( *str != '\0' && *str == '0' ) { ++str; }

	String newStr( str );
	Int32 val = 0;
	FromString< Int32 >( newStr, val );
	return val;
}

// Conversion from String for GameTime
template<> RED_INLINE Bool FromString( const String& text, GameTime& value )
{
	TDynArray< String > vals = text.Split( TXT(":") );

	if ( vals.Size() == 0 )
	{
		value.m_seconds = 0;
	}
	else if ( vals.Size() == 1 )
	{
		value.m_seconds = ReadZeroPaddedInt( vals[ 0 ] );
	}
	else if ( vals.Size() == 2 )
	{
		Int32 hours, minutes;
		hours = ReadZeroPaddedInt( vals[ 0 ] );
		minutes = ReadZeroPaddedInt( vals[ 1 ] );
		value.m_seconds = 60 * (60 * hours + minutes);
	}
	else if ( vals.Size() == 3 )
	{
		Int32 days, hours, minutes;
		days = ReadZeroPaddedInt( vals[ 0 ] );
		hours = ReadZeroPaddedInt( vals[ 1 ] );
		minutes = ReadZeroPaddedInt( vals[ 2 ] );
		value.m_seconds = 60 * (60 * ( 24 * days + hours ) + minutes);
	}
	else if ( vals.Size() == 4 )
	{
		Int32 days, hours, minutes, seconds;
		days = ReadZeroPaddedInt( vals[ 0 ] );
		hours = ReadZeroPaddedInt( vals[ 1 ] );
		minutes = ReadZeroPaddedInt( vals[ 2 ] );
		seconds = ReadZeroPaddedInt( vals[ 3 ] );
		value.m_seconds = 60 * (60 * ((24 * days) + hours) + minutes) + seconds;
	}
	else
	{
		ERR_ENGINE( TXT("FromString - unknown time format") );
		return false;
	}
	//LOG_ENGINE( TXT("GameTime: string %s read as %d"), text.AsChar(), value.m_seconds );
	return true;
}
