/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SItemUniqueId
{
	DECLARE_RTTI_STRUCT( SItemUniqueId );

	typedef Uint32 TValue;
	static const SItemUniqueId INVALID;	//!< Invalid id

	SItemUniqueId()
		: m_value(0)
	{
	}

	explicit SItemUniqueId( TValue uniqueIdValue )
		: m_value(uniqueIdValue)
	{
	}

	RED_INLINE Bool operator==( SItemUniqueId otherUniqueId ) const
	{
		return m_value == otherUniqueId.m_value;
	}

	RED_INLINE Bool operator!=( SItemUniqueId otherUniqueId ) const
	{
		return m_value != otherUniqueId.m_value;
	}

	//! Check if valid
	RED_INLINE operator Bool() const
	{
		return m_value != 0;
	}

	RED_INLINE TValue GetValue() const { return m_value; }

	void Load( IGameLoader* loader );
	void Save( IGameSaver* saver );

	void StreamLoad( ISaveFile* loader, Uint32 version );
	void StreamSave( ISaveFile* saver );

	RED_INLINE Uint32 CalcHash() const { return ::GetHash( m_value ); }

private:
	TValue m_value;
};

BEGIN_CLASS_RTTI( SItemUniqueId );	
	PROPERTY( m_value );	
END_CLASS_RTTI();

struct SItemUniqueIdGenerator
{
	DECLARE_RTTI_STRUCT( SItemUniqueIdGenerator );

	SItemUniqueIdGenerator() : m_counter(0) {}
	SItemUniqueId GenerateNewId() { return SItemUniqueId( ++m_counter ); }

	void Load( IGameLoader* loader );
	void Save( IGameSaver* saver );

	void StreamLoad( ISaveFile* loader, Uint32 version );
	void StreamSave( ISaveFile* saver );

	void Reset() { m_counter = 0; }
private:
	SItemUniqueId::TValue m_counter;
};

BEGIN_CLASS_RTTI( SItemUniqueIdGenerator );	
	PROPERTY( m_counter );
END_CLASS_RTTI();