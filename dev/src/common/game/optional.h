/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// a simple version of Boost.Optional
template <typename T>
class TOptional
{
public:

	TOptional()
		: m_initialized( false )
	{}

	TOptional( const T& value )
		: m_initialized( true )
		, m_value( value )
	{}

	RED_INLINE Bool IsInitialized() const
	{
		return m_initialized;
	}

	RED_INLINE const T& Get() const
	{
		return m_value;
	}

	RED_INLINE void Set( const T& value )
	{
		m_initialized = true;
		m_value = value;
	}

private:

	T		m_value;
	Bool	m_initialized;
};