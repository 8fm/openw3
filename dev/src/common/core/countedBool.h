/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Counted bool, has a value of false only when internal counter==0
class CountedBool
{
private:
	Red::Threads::CAtomic< Int32 >		m_value;		//!< Internal value, thread safe

public:
	//! Default constructor
	RED_INLINE CountedBool()
		: m_value( 0 )
	{};

	//! Construct from boolean
	RED_INLINE CountedBool( Bool value )
		: m_value( value ? 1 : 0 )
	{};

	//! Copy constructor
	RED_INLINE CountedBool( const CountedBool& other )
		: m_value( other.m_value.GetValue() )
	{};

	//! Assignment operator
	RED_INLINE CountedBool& operator=( const CountedBool& other )
	{
		m_value.SetValue( other.m_value.GetValue() );
		return *this;
	}

	//! Convert to bool
	RED_INLINE operator Bool() const
	{
		return m_value.GetValue() > 0;
	}

public:
	//! Set value ( increment internal counter )
	RED_INLINE void Set()
	{
		ASSERT( m_value.GetValue() >= 0 );
		m_value.Increment();
	}

	//! Unset value ( decrement internal counter )
	RED_INLINE void Unset()
	{
		ASSERT( m_value.GetValue() > 0 );
		m_value.Decrement();
	}

	//! Update, true increments, false decrements, returns final state
	RED_INLINE Bool Update( Bool status )
	{
		if ( status )
		{
			Set();
		}
		else
		{
			Unset();
		}

		return m_value.GetValue() > 0;
	}
};