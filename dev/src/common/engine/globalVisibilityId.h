/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//------------------------------------------------------------

/// Global (world wide) visibility ID for an object
class GlobalVisID
{
public:
	GlobalVisID();
	GlobalVisID( const class CResource* resource, const struct Matrix& localToWorld );
	GlobalVisID( const class CResource* resource, const class CComponent* comp );
	GlobalVisID( const Uint32 modelId, const struct Matrix& localToWorld );
	GlobalVisID( const GlobalVisID& other );

	RED_INLINE const Uint64 GetKey() const
	{
		return m_id;
	}

	RED_INLINE const Uint32 CalcHash() const
	{
		return (Uint32)( m_id ^ (m_id >> 32) );
	}

	RED_INLINE const Bool IsValid() const
	{
		return m_id != 0;
	}

	RED_INLINE const Bool operator==( const GlobalVisID& other ) const
	{
		return m_id == other.m_id;
	}

	RED_INLINE friend IFile& operator<<( IFile& file, GlobalVisID& id )
	{
		file << id.m_id;
		return file;
	}

	// Get human readable string
	String ToString() const;

	// Set debug string
	void SetDebugString( const String& debugString );

private:
	Uint64	m_id;

#ifndef RED_FINAL_BUILD
	String	m_debugString;
#endif
};

//------------------------------------------------------------

RED_DECLARE_RTTI_NAME( GlobalVisID );

class CSimpleRTTITypeGlobalVisID : public TSimpleRTTIType< GlobalVisID >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< GlobalVisID* >(object)->~GlobalVisID();
	}

	virtual const CName& GetName() const
	{
		return CNAME( GlobalVisID );
	}

	virtual ERTTITypeType GetType() const
	{
		return RT_Simple;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = static_cast< const GlobalVisID* >(data)->ToString();
		return true;
	}

	virtual Bool FromString( void*, const String& ) const
	{
		return false;
	}
};

template<>
struct TTypeName< GlobalVisID >
{													
	static const CName& GetTypeName() { return CNAME( GlobalVisID ); }	
};

//------------------------------------------------------------
