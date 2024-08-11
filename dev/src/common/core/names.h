/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/typetraits.h"
#include "../redSystem/nameHash.h"
#include "namesPool.h"
#include "hash.h"


/// Indexed name
class CName
{
public:
	typedef Red::CNameHash	CNameHash;

public:
	static const CName NONE;
	
public:
	typedef	CNamesPool::TIndex TIndex;

private:
	TIndex		m_index;

public:
				CName()
					: m_index( CNamesPool::INDEX_NONE ) {}
				CName( const CName& other )
					: m_index( other.m_index ) {}
	explicit	CName( TIndex index )
					: m_index( index ) {}
	explicit	CName( const Char* name ) { Set( name ); }
	explicit	CName( const String& name ) { Set( name ); }
			
				
				~CName() {}

public:
	// Otherwise somebody could incorrectly call with CName( UNICODE_TO_ANSI("...") ) then the string would be converted to a CNameHash
	// but without a name having been set first. It'll at least be a compiler error now. The CNameHash needs to be non-explicit, unfortunately.
	static CName CreateFromHash( CNameHash nameHash );

private:
	explicit	CName( CNameHash nameHash );

public:
	void		Set( const Char* name );
	void		Set( const String& name );

public:
	CName& operator=( const CName& rhs ) { m_index = rhs.m_index; return *this; }

public:
	Bool operator==( const CName& rhs ) const { return m_index == rhs.m_index; }
	Bool operator==( const Char* rhs ) const { CName name( rhs ); return *this == name; } //TBD: Could be a straight str(i)cmp, but not if we move away from saving string for all cnames

	Bool operator!=( const CName& rhs ) const { return m_index != rhs.m_index; }
	Bool operator!=( const Char* rhs ) const { CName name( rhs ); return *this != name; } //TBD: Could be a straight str(i)cmp, but not if we move away from saving string for all cnames

	Bool operator<( const CName& rhs ) const { return m_index < rhs.m_index; }
	Bool operator>( const CName& rhs ) const { return m_index > rhs.m_index; }

	operator Bool() const { return m_index != CNamesPool::INDEX_NONE; }

public:
	CNameHash	GetSerializationHash() const;

public:
	const Char* AsChar() const;
	const AnsiChar* AsAnsiChar() const;

	//TEMP: to avoid making a trillion minor changes in this integration changelist 
	String AsString() const { return AsChar(); }

	TIndex GetIndex() const { return m_index; }
	
	Bool Empty() const { return m_index == CNamesPool::INDEX_NONE; }

	RED_FORCE_INLINE Uint32 CalcHash() const { return m_index; }

private:
	Bool IsNone( const Char* name ) const { return !name || !*name || Red::System::StringCompare( name, MACRO_TXT(RED_CNAME_NONETXT) ) == 0; }
	Bool IsNone( const String& name ) const { return name.Empty() || name == MACRO_TXT(RED_CNAME_NONETXT); }
};

// This is here so that the default GetHash() function isn't used (because it passes CName by reference incurring unnecessary cost)
RED_FORCE_INLINE Uint32 GetHash( const CName name ) { return name.GetIndex(); }

// Since we use reference counting for names we cannot copy them directly
// That's a lie [DEX, W3]
//template <> struct TCopyableType<CName>	{ enum { Value = false }; };
