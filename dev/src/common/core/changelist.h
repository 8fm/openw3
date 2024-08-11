/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

/// Source control changelist (this is a struct because the source control
/// may change in the future and use hashes instead of simple Uint32s)
class SChangelist
{
	friend class ISourceControl;

	// This thing right here may change at some point, if you depend on it being
	// Uint32 forever it will be your doom
	typedef Uint32 ID;

private:
	ID m_id;

	RED_INLINE Bool IsEqualTo( const SChangelist& changelist ) const
	{
		return m_id == changelist.m_id;
	}

	SChangelist() : m_id( 0 ) {}
	explicit SChangelist( ID id ) : m_id( id ) {};

public:
	RED_INLINE SChangelist( const SChangelist& changelist )
	{
		m_id = changelist.m_id;
	}

	RED_INLINE Bool operator == ( const SChangelist& changelist ) const
	{
		return IsEqualTo( changelist );
	}

	RED_INLINE Bool operator != ( const SChangelist& changelist ) const
	{
		return !IsEqualTo( changelist );
	}

	// Represents the default changelist
	static const SChangelist DEFAULT;
};

#endif
