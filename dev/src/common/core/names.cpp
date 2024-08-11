/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "names.h"
#include "namesPool.h"

const CName CName::NONE;

CName::CName( CNameHash nameHash )
{
	//TODO: Instrument branch prediction vs just getting from pool
	// This also catches if somebody passes in CNameHash::Hash("None")
	if ( nameHash.GetValue() == CNameHash::INVALID_HASH_VALUE || nameHash == CNameHash(RED_CNAME_NONETXT) )
	{
		m_index = CNamesPool::INDEX_NONE;
	}
	else
	{
		m_index = SNamesPool::GetInstance().GetIndexFromHash( nameHash );
	}

#ifndef RED_FINAL_BUILD
	RED_ASSERT( SNamesPool::GetInstance().FindText( m_index ) != nullptr, TXT("CName created with hash, but has no CName for it has been created by a text value first") );
#endif
}

CName CName::CreateFromHash( CNameHash nameHash )
{
	return CName( nameHash );
}

void CName::Set( const Char* name )
{
	//TODO: Instrument vs just getting from pool (with addition aliases "" and '\0' for None)
	if ( IsNone( name ) )
	{
		m_index = CNamesPool::INDEX_NONE;
		return;
	}

	m_index = SNamesPool::GetInstance().AddDynamicPoolName( name );
	RED_ASSERT( m_index != CNamesPool::INDEX_NONE );
}

void CName::Set( const String& name )
{
	if ( IsNone( name ) )
	{
		m_index = CNamesPool::INDEX_NONE;
		return;
	}

	m_index = SNamesPool::GetInstance().AddDynamicPoolName( name.AsChar() );
	RED_ASSERT( m_index != CNamesPool::INDEX_NONE );
}

const Char* CName::AsChar() const
{
	const Char* retval = SNamesPool::GetInstance().FindText( m_index );

	//TBD: CName created by hash but never had a name entry...
	RED_ASSERT( retval );
	if ( ! retval )
	{
		return MACRO_TXT(RED_CNAME_NONETXT);
	}

	return retval;
}

const AnsiChar* CName::AsAnsiChar() const
{
	return SNamesPool::GetInstance().FindTextAnsi( m_index );
}

CName::CNameHash CName::GetSerializationHash() const
{
	return SNamesPool::GetInstance().GetSerializationHash( m_index );
}
