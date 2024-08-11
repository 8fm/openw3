/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IdTag system ( a.k.a. PESEL system )
// Static gameplay entities use preallocated static IdTags that are initialized in the editor when CGameplayEntity is placed on the level.
// Dynamic entities that can be spawned in the level ( NPCs, loots, traps, etc ) should use preallocated IdTagPool.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

/// ID tag
class IdTag
{
	friend class CIdTagManager;

private:
	CGUID	m_guid;			//!< Preallocated GUID
	Bool	m_isDynamic;	//!< Is this a dynamically assigned IdTag

public:
	//! Default constructor
	RED_INLINE IdTag()
		: m_isDynamic( false )
	{};

	//! Copy constructor
	RED_INLINE IdTag( const IdTag& other )
		: m_isDynamic( other.m_isDynamic )
		, m_guid( other.m_guid )
	{};

	//! Is this an empty tag 
	RED_INLINE Bool IsValid() const { return !m_guid.IsZero(); }

	//! Is this a dynamic tag ?
	RED_INLINE Bool IsDynamic() const { return m_isDynamic; }

	//! Compare with other tag, returns true if the same, checks category
	RED_INLINE Bool operator==( const IdTag& other ) const
	{
		return ( other.m_guid == m_guid ) && ( other.m_isDynamic == m_isDynamic );
	}

	//! Less operator, needed for THashMap
	RED_INLINE Bool operator<( const IdTag& other ) const
	{
		return m_guid < other.m_guid;
	}

	//! Serialize
	RED_INLINE friend IFile& operator<<( IFile& file, IdTag& tag )
	{
		tag.Serialize( file );
		return file;
	}

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		return m_guid.CalcHash(); // Exclude m_isDynamic from hash calculation to benefit from 32-bit based hash calculation; it's either true or false
	}

public:
	//! Serialize
	void Serialize( IFile& file );

	//! Convert to string
	void ToString( String& outString ) const;

	const CGUID& GetGuid() const { return m_guid; }

public:
	//! Allocate static IdTag, to be used by static gameplay entities
	static IdTag AllocateStaticTag();

	//! Get the empty IdTag
	static const IdTag& Empty();
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////

RED_DECLARE_RTTI_NAME( IdTag );

class CSimpleRTTITypeIdTag : public TSimpleRTTIType< IdTag >
{
public:
	virtual const CName& GetName() const
	{
		return CNAME( IdTag );
	}

	virtual ERTTITypeType GetType() const
	{
		return RT_Simple;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		const IdTag* tag = static_cast< const IdTag* >(data);
		tag->ToString( valueString );
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		return false;
	}
};

template<>
struct TTypeName< IdTag >
{													
	static const CName& GetTypeName() { return CNAME( IdTag ); }	
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////

