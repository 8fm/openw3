/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

///////////////////////////////////////////////////////////////////////////////////////////

// Forward declaration
class IEntityHandleData;

/// Savable handle to entity
class EntityHandle
{
private:
	IEntityHandleData*		m_data;			//!< Entity handle data

public:
	//! Is the handle NULL
	RED_INLINE Bool Empty() const { return m_data == NULL; }

public:
	//! Default constructor
	RED_INLINE EntityHandle()
		: m_data( NULL )
	{};

	//! Destructor
	~EntityHandle();

	//! Copy constructor
	EntityHandle( const EntityHandle& other );

	//! Compare
	Bool operator==( const EntityHandle& other ) const;

	//! Compare
	Bool operator<( const EntityHandle& other ) const;

	//! Assign
	EntityHandle& operator=( const EntityHandle& other );

	//! Serialize
	void Serialize( IFile& file );

	const CGUID& GetEntityHandleGUID() const;

	const IdTag& GetEntityHandleIdTag() const;

public:
	//! Set from entity
	void Set( CEntity* entity );

	//! Get entity
	CEntity* Get();

	//! Get entity ( const version )
	const CEntity* Get() const;

	//! To string representation
	String ToString() const;

public:
	//! Serialization operator
	friend IFile& operator<<( IFile& file, EntityHandle& handle )
	{
		handle.Serialize( file );
		return file;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////

RED_DECLARE_RTTI_NAME( EntityHandle );

class CSimpleRTTITypeEntityHandle : public TSimpleRTTIType< EntityHandle >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< EntityHandle* >(object)->~EntityHandle();
	}

	virtual const CName& GetName() const
	{
		return CNAME( EntityHandle );
	}

	virtual ERTTITypeType GetType() const
	{
		return RT_Simple;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		EntityHandle* handle = (EntityHandle*)data;
		valueString = handle->ToString();
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		return false;
	}
};

template<>
struct TTypeName< EntityHandle >
{													
	static const CName& GetTypeName() { return CNAME( EntityHandle ); }	
};

///////////////////////////////////////////////////////////////////////////////////////////
