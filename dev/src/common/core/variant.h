/**
* Copyright ɠ2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "names.h"
#include "rttiType.h"

/// Variant variable - can hold any value supported by RTTI
class CVariant
{
protected:
	CName			m_type;					//!< Name of the type
	IRTTIType*		m_rttiType;				//!< RTTI type used to do all the stuff
	void*			m_data;					//!< Data. Data is allocated via the heap, or a pointer to const data passed to explicitly called Init (m_holdStaticPointer)
	bool			m_holdStaticPointer;	//!< m_data points to static data (not owned by CVariant)

public:
	//! Get type
	RED_INLINE CName GetType() const { return m_type; }

	//! Is valid ? ( has type defined )
	RED_INLINE Bool IsValid() const { return m_rttiType != NULL && m_data != NULL; }

	//! Access the internal property
	RED_INLINE IRTTIType* GetRTTIType() const { return m_rttiType; }

	//! Get raw data
	RED_INLINE const void* GetData() const { return m_data; }

	//! Get raw data for modification ( use with care )
	RED_INLINE void* GetData() { return m_data; }

	//! Get raw data size
	Uint32 GetDataSize() const;

public:
	//! Empty constructor
	RED_INLINE CVariant()
		: m_rttiType( NULL )
		, m_data( NULL )
		, m_holdStaticPointer( false )
	{
	}

	//! Simple type constructors
	template< typename T >
	RED_INLINE explicit CVariant( const T& value )
		: m_rttiType( NULL )
		, m_data( NULL )
		, m_holdStaticPointer( false )
	{
		CName n( GetTypeName<T>() );
		Init( n, &value );
	}

	//! Copy constructor
	RED_INLINE CVariant( const CVariant& other )
		: m_rttiType( NULL )
		, m_data( NULL )
		, m_holdStaticPointer( false )
	{
		if ( other.GetType() )
		{
			Init( other.GetType(), other.GetData() );
		}
	}

	//! Typed constructor
	RED_INLINE CVariant( const CName& typeName, const void* data )
		: m_rttiType( NULL )
		, m_data( NULL )
		, m_holdStaticPointer( false )
	{
		Init( typeName, data );
	}

	//! Cleanup
	~CVariant();

	//! Assignment
	CVariant& operator=( const CVariant& other );

	//! Comparison
	Bool operator==( const CVariant& other ) const;
	Bool operator!=( const CVariant& other ) const;

public:	
	//! Extract value using RTTI system
	template< class T >
	RED_INLINE Bool AsType( T& value ) const
	{
		// Check type and extract value
		IRTTIType* targetType = GetTypeObject< T >();
		if ( SRTTI::GetInstance().CanCast( m_rttiType, targetType ) )
		{
			ASSERT( m_rttiType->GetSize() == sizeof(T) );
			value = * ( const T* ) m_data;
			return true;
		}

		// Not valid type
		return false;
	}

	/*//! Check type
	template< class T >
	RED_INLINE Bool IsA() const
	{
		// Check type and extract value
		if ( m_rttiType && m_type == GetTypeName<T>() )
		{
			ASSERT( m_rttiType->GetSize() == sizeof(T) );
			return true;
		}

		// Not the same type
		return false;
	}*/

public:
	//! Destroy variant value
	void Clear();

	//! Initialize variant with data
	void Init( CName typeName, const void* initialData, bool holdStaticPointer = false );

public:
	//! Is this variant an array ? ( arrays can be accessed via CVariantArray ) 
	Bool IsArray() const;

	//! Convert value to string, returns false on error
	String ToString() const;

	//! Convert value to string, returns false on error
	Bool ToString( String& value ) const;

	//! Set value from string, returns false on error
	Bool FromString( const Char* value );

	//! Set value from string, returns false on error
	Bool FromString( const String& value );

public:
	//! Serialization
	void Serialize( IFile& file );

public:
	//! Serialization operator
	RED_INLINE friend IFile& operator<<( IFile& file, CVariant& variant )
	{
		variant.Serialize( file );
		return file;
	}
};

//////////////////////////////////////////////////////////////////////////
// variant type.

class CSimpleRTTITypeVariant : public TSimpleRTTIType< CVariant >
{
public:

	virtual void Destruct( void *object ) const
	{
		static_cast< CVariant* >(object)->~CVariant();
	}

	virtual const CName& GetName() const { return CNAME( CVariant ); }

	virtual ERTTITypeType GetType() const { return RT_Simple; }

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = String() + TXT("[") + 
			( ( const CVariant *) data )->GetType().AsChar() + TXT("] ") +
			( ( const CVariant *) data )->ToString();
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		static_cast< CVariant* >( data )->FromString( valueString );
		return true;
	}
};

template<>
struct TTypeName< CVariant >
{													
	static const CName& GetTypeName() { return CNAME( CVariant ); }	
};
