/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "string.h"

/// File context info for script data
class CScriptFileContext
{
public:
	String		m_file;		//!< Referenced file
	Int32			m_line;		//!< Referenced line

public:
	RED_INLINE CScriptFileContext()
		: m_line( -1 )
	{};

	//! Package context info
	RED_INLINE  CScriptFileContext( const String &file, Int32 line )
		: m_file( file )
		, m_line( line )
	{};

	//! Copy
	RED_INLINE CScriptFileContext( const CScriptFileContext& other )
		: m_file( other.m_file )
		, m_line( other.m_line )
	{}

	//! Move
	RED_INLINE CScriptFileContext( CScriptFileContext&& other )
		: m_file( Move( other.m_file ) )
		, m_line( Move( other.m_line ) ) 
	{}

	//! Assign
	RED_INLINE CScriptFileContext& operator=( const CScriptFileContext& other )
	{
		if ( this != &other )
		{
			m_file = other.m_file;
			m_line = other.m_line;
		}
		return *this;
	}

	//! Move
	RED_INLINE CScriptFileContext& operator=( CScriptFileContext&& other )
	{
		if ( this != &other )
		{
			m_file = Move( other.m_file );
			m_line = Move( other.m_line );
		}
		return *this;
	}

public:
	//! Convert to string
	RED_INLINE  String ToString() const
	{
		if ( m_line != -1 )
		{
			return String::Printf( TXT("%ls[%i]"), m_file.AsChar(), m_line );
		}
		else if ( !m_file.Empty() )
		{
			return String::Printf( TXT("%ls[]"), m_file.AsChar() );
		}
		else
		{
			return TXT("Unknown");
		}
	}
};