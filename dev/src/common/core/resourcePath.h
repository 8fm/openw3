/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"
#include "string.h"

/// Path to resource
class CResourcePath
{
protected:
	TDynArray< AnsiChar, MC_Engine >	m_path;

public:	
	RED_INLINE CResourcePath()
	{};

	RED_INLINE CResourcePath( const Char* str )
	{
		SetPath( str );
	}

	RED_INLINE CResourcePath( const String& str )
	{
		SetPath( str.AsChar() );
	}

	RED_INLINE CResourcePath( const CResourcePath& path )
		: m_path( path.m_path )
	{
	};

	RED_INLINE CResourcePath& operator=( const CResourcePath& path )
	{
		m_path = path.m_path;
		return *this;
	}

	RED_INLINE CResourcePath& operator=( const String& path )
	{
		SetPath( path.AsChar() );
		return *this;
	}

	RED_INLINE CResourcePath& operator=( const Char* path )
	{
		SetPath( path );
		return *this;
	}

	RED_INLINE Bool operator==( const CResourcePath& other ) const
	{
		return Red::System::MemoryCompare( m_path.Data(), other.m_path.Data(), m_path.Size() ) == 0;
	}

	RED_INLINE Bool operator!=( const CResourcePath& other ) const
	{
		return Red::System::MemoryCompare( m_path.Data(), other.m_path.Data(), m_path.Size() ) != 0;
	}

	RED_INLINE Bool operator<( const CResourcePath& other ) const
	{
		return Red::System::MemoryCompare( m_path.Data(), other.m_path.Data(), m_path.Size() ) < 0;
	}

	RED_INLINE Bool operator>( const CResourcePath& other ) const
	{
		return Red::System::MemoryCompare( m_path.Data(), other.m_path.Data(), m_path.Size() ) > 0;
	}

	RED_INLINE Bool operator<=( const CResourcePath& other ) const
	{
		return Red::System::MemoryCompare( m_path.Data(), other.m_path.Data(), m_path.Size() ) <= 0;
	}

	RED_INLINE Bool operator>=( const CResourcePath& other ) const
	{
		return Red::System::MemoryCompare( m_path.Data(), other.m_path.Data(), m_path.Size() ) >= 0;
	}

	RED_INLINE operator Bool() const
	{
		return m_path.Size() > 0;
	}

	RED_INLINE const AnsiChar* ToAnsiString() const
	{
		return m_path.Empty() ? "" : &m_path[0];
	}
			
public:
	friend IFile& operator<<( IFile& file, CResourcePath& path )
	{
		file << path.m_path;
		return file;
	}

public:
	void SetPath( const Char* path );
	String ToString() const;
};