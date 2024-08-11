/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"

/*******************************************/
/* Data buffer for holding property value  */
/*******************************************/
class CPropertyDataBuffer : public TDynArray< Uint8 >
{
public:
	IProperty*	m_property;

public:
	//! Constructor
	FORCE_INLINE CPropertyDataBuffer( IProperty* prop )
		: m_property( prop )
	{
		ASSERT( prop );
		Resize( prop->Size() );
		AMemSet( Data(), 0, prop->Size() );
	}

    //! Copy constructor
    FORCE_INLINE CPropertyDataBuffer( const CPropertyDataBuffer &buf )
        : m_property( buf.m_property )
    {
        Resize( m_property->Size() );
        AMemSet( Data(), 0, m_property->Size() );
        m_property->Copy( Data(), buf.Data() );
    }

    //! Copy operator
    FORCE_INLINE const CPropertyDataBuffer &operator=( const CPropertyDataBuffer &buf )
    {
        m_property->Clean( Data() );

        m_property = buf.m_property;
        Resize( m_property->Size() );
        AMemSet( Data(), 0, m_property->Size() );
        m_property->Copy( Data(), buf.Data() );
        return *this;
    }

	//! Cleanup
	FORCE_INLINE ~CPropertyDataBuffer()
	{
		m_property->Clean( Data() );
	}

	//! Convert to variant
	FORCE_INLINE class CVariant ToVariant() const;
};

/************************/
/* Simple type property */
/************************/
template < class T >
class TSimpleProperty : public IProperty
{
public:
	TSimpleProperty( CStruct* owner, Uint32 offset, EFieldType type, const CName& name, const String& hint, Uint32 flags, const String& customEditor = TXT("") )
		: IProperty( owner, offset, type, name, hint, flags, customEditor )
	{};

	// Get property size
	virtual Uint Size() const
	{
		return sizeof(T);
	}

	// Compare property value to other value
	virtual Bool Compare( const void* data1, const void* data2, Uint flags ) const
	{
		(void)flags;
		return *( const T* ) data1 == *( const T*) data2;
	}

	// Copy property value
	virtual void Copy( void* dest, const void* src ) const
	{
		*( T* ) dest = *( const T* ) src;		
	}

	// Clean property value
	virtual void Clean( void* data ) const
	{
		(void)data;
		(( T* ) data)->~T(); 
	}

	// Serialize property value to IFile stream
	virtual Bool Serialize( IFile& file, void* data ) const
	{
		file << (*(T*) data);
		return true;
	}

	// Export property value to string
	virtual Bool Export( const void* data, String& valueString ) const
	{
		valueString = ToString<T>( *( const T*) data );
		return true;
	}

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const
	{
		return FromString<T>( valueString, *(T*) data );
	}

	// Serialize to/from XML file
	virtual Bool SerializeXML( IFile& file, void* data ) const
	{
		return IProperty::SerializeXML( file, data );
	}

	// Returns true if property should be walked by GC
	virtual Bool IsGCRelevant() const
	{
		return false;
	}

	// Get type name
	virtual CName GetTypeName() const
	{
		return TypeID< T >();
	}
};

/************************/
/* Integer property		*/
/************************/
class CIntProperty : public TSimpleProperty< Int >
{
public:
	Int			m_min;		// Minimum allowed value
	Int			m_max;		// Maximum allowed value

public:
	CIntProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, Int minRange = -INT_MAX, Int maxRange = INT_MAX, const String& customEditor = TXT("") )
		: TSimpleProperty< Int >( owner, offset, FT_IntProperty, name, hint, flags, customEditor )
		, m_min( minRange )
		, m_max( maxRange )
	{};

	FORCE_INLINE Bool IsRangeDefined() const
	{
		return (m_min > -INT_MAX) || (m_max < INT_MAX);
	}

	FORCE_INLINE Int Clamp( Int value ) const
	{
		return ::Clamp( value, m_min, m_max );
	}

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const
	{
		Int& intData = * ( Int* ) data;
		if ( FromString<Int>( valueString, intData ) )
		{
			intData = Clamp( intData );
			return true;
		}
		return false;
	}
};

/************************/
/* Byte property		*/
/************************/
class CByteProperty : public TSimpleProperty< Uint8 >
{
public:
	CByteProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, const String& customEditor = TXT("") )
		: TSimpleProperty< Uint8 >( owner, offset, FT_ByteProperty, name, hint, flags, customEditor )
	{};
};

/************************/
/* Bool property		*/
/************************/
class CBoolProperty : public TSimpleProperty< Bool >
{
public:
	CBoolProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, const String& customEditor = TXT("") )
		: TSimpleProperty< Bool >( owner, offset, FT_BoolProperty, name, hint, flags, customEditor )
	{};
};

/************************/
/* Float property		*/
/************************/
class CFloatProperty : public TSimpleProperty< Float >
{
public:
	Float		m_min;		// Minimum allowed value
	Float		m_max;		// Maximum allowed value

public:
	CFloatProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, Float minRange = -FLT_MAX, Float maxRange = FLT_MAX, const String& customEditor = TXT("") )
		: TSimpleProperty< Float >( owner, offset, FT_FloatProperty, name, hint, flags, customEditor )
		, m_min( minRange )
		, m_max( maxRange )
	{};

	FORCE_INLINE Bool IsRangeDefined() const
	{
		return (m_min > -FLT_MAX) || (m_max < FLT_MAX);
	}

	FORCE_INLINE Float Clamp( Float value ) const
	{
		return ::Clamp( value, m_min, m_max );
	}

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const
	{
		Float& floatData = * ( Float* ) data;
		if ( FromString<Float>( valueString, floatData ) )
		{
			floatData = Clamp( floatData );
			return true;
		}
		return false;
	}
};

/************************/
/* Double property		*/
/************************/
class CDoubleProperty : public TSimpleProperty< Double >
{
public:
	CDoubleProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, const String& customEditor = TXT("") )
		: TSimpleProperty< Double >( owner, offset, FT_DoubleProperty, name, hint, flags, customEditor )
	{};
};

/************************/
/* String property		*/
/************************/
class CStringProperty : public TSimpleProperty< String >
{
public:
	CStringProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, const String& customEditor = TXT("") )
		: TSimpleProperty< String >( owner, offset, FT_StringProperty, name, hint, flags, customEditor )
	{};

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const
	{
		*( String *) data = valueString;
		return true;
	}
};

/************************/
/* Name property		*/
/************************/
class CNameProperty : public TSimpleProperty< CName >
{
public:
	CNameProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, const String& customEditor = TXT("") )
		: TSimpleProperty< CName >( owner, offset, FT_NameProperty, name, hint, flags, customEditor )
	{};

	// Export property value to string
	virtual Bool Export( const void* data, String& valueString ) const
	{
		valueString = ( ( const CName *) data )->AsChar();
		return true;
	}

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const
	{
		* ( ( CName* ) data ) = CName( valueString );
		return true;
	}
};

/************************/
/* Variant property		*/
/************************/
class CVariantProperty : public TSimpleProperty< CVariant >
{
public:
	CVariantProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, const String& customEditor = TXT("") )
		: TSimpleProperty< CVariant >( owner, offset, FT_NameProperty, name, hint, flags, customEditor )
	{};

	// Export property value to string
	virtual Bool Export( const void* data, String& valueString ) const
	{
		valueString = String() + TXT("[") + 
			( ( const CVariant *) data )->GetType().AsChar() + TXT("] ") +
			( ( const CVariant *) data )->ToString();
		return true;
	}

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const
	{
		( ( CVariant* ) data )->FromString(valueString);
		return true;
	}
};

/************************/
/* Object property		*/
/************************/
class CObjectProperty : public TSimpleProperty< CObject* >
{
protected:
	CClass*		m_class;

public:
	// Get property object class
	FORCE_INLINE CClass* GetClass() const { return m_class; }

public:
	CObjectProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, CClass* objectClass, const String& customEditor = TXT("") )
		: TSimpleProperty< CObject* >( owner, offset, FT_ObjectProperty, name, hint, flags, customEditor )
		, m_class( objectClass )
	{};

	// Compare property value to other value
	virtual Bool Compare( const void* data1, const void* data2, Uint flags ) const;

	// Export property value to string
	virtual Bool Export( const void* data, String& valueString ) const;

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const;

	// Serialize to/from XML file
	virtual Bool SerializeXML( IFile& file, void* data ) const;

	// Returns true if property should be walked by GC
	virtual Bool IsGCRelevant() const { return true; }

	// Get type name
	virtual CName GetTypeName() const;

	//! Check type match, used to check if script type definition matches native one
	virtual Bool CompareType( IField* field, String& error ) const;
};

/************************/
/* Enum property		*/
/************************/
class CEnumProperty : public TSimpleProperty< Uint8 >
{
protected:
	CEnum*		m_enum;

public:
	//! Get enumeration
	FORCE_INLINE CEnum* GetEnum() const { return m_enum; }

public:
	CEnumProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, CEnum* enumObject, const String& customEditor = TXT("") )
		: TSimpleProperty< Uint8 >( owner, offset, FT_EnumProperty, name, hint, flags, customEditor )
		, m_enum( enumObject )
	{};

	// Get property size
	virtual Uint Size() const;

	// Serialize property value to IFile stream
	virtual Bool Serialize( IFile& file, void* data ) const;

	// Compare property value to other value
	virtual Bool Compare( const void* data1, const void* data2, Uint flags ) const;

	// Copy property value
	virtual void Copy( void* dest, const void* src ) const;

	// Clean property value
	virtual void Clean( void* data ) const;

	// Export property value to string
	virtual Bool Export( const void* data, String& valueString ) const;

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const;

	// Get type name
	virtual CName GetTypeName() const;

	//! Check type match, used to check if script type definition matches native one
	virtual Bool CompareType( IField* field, String& error ) const;
};

/************************/
/* Struct property		*/
/************************/
class CStructProperty : public IProperty
{
protected:
	CStruct*		m_struct;			

public:
	// Get struct type
	FORCE_INLINE CStruct* GetStruct() const { return m_struct; }

public:
	CStructProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, CStruct* structObject, const String& customEditor = TXT("") )
		: IProperty( owner, offset, FT_StructProperty, name, hint, flags, customEditor )
		, m_struct( structObject )
	{};

	// Get property size
	virtual Uint Size() const;

	// Compare property value to other value
	virtual Bool Compare( const void* data1, const void* data2, Uint flags ) const;

	// Copy property value
	virtual void Copy( void* dest, const void* src ) const;

	// Clean property value
	virtual void Clean( void* data ) const;

	// Serialize property value to IFile stream
	virtual Bool Serialize( IFile& file, void* data ) const;

	// Export property value to string
	virtual Bool Export( const void* data, String& valueString ) const;

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const;

	// Serialize to/from XML file
	virtual Bool SerializeXML( IFile& file, void* data ) const;

	// Returns true if property should be walked by GC
	virtual Bool IsGCRelevant() const;

	// Get type name
	virtual CName GetTypeName() const;

	//! Check type match, used to check if script type definition matches native one
	virtual Bool CompareType( IField* field, String& error ) const;
};

/********************************/
/* Dynamic array property		*/
/********************************/
class CArrayProperty : public IProperty
{
protected:
	IProperty*		m_innerProperty;

public:
	CArrayProperty( CStruct* owner, Uint32 offset, const CName& name, const String& hint, Uint32 flags, IProperty* innerProperties, const String& customEditor = TXT("") )
		: IProperty( owner, offset, FT_ArrayProperty, name, hint, flags, customEditor )
		, m_innerProperty( innerProperties )
	{};

	~CArrayProperty()
	{
		delete m_innerProperty;
	}

	FORCE_INLINE IProperty* GetInnerProperty() const { return m_innerProperty; }

	// Get property size
	virtual Uint Size() const;

	// Compare property value to other value
	virtual Bool Compare( const void* data1, const void* data2, Uint flags ) const;

	// Copy property value
	virtual void Copy( void* dest, const void* src ) const;

	// Clean property value
	virtual void Clean( void* data ) const;

	// Serialize property value to IFile stream
	virtual Bool Serialize( IFile& file, void* data ) const;

	// Export property value to string
	virtual Bool Export( const void* data, String& valueString ) const;

	// Import property value from string
	virtual Bool Import( void* data, const String& valueString ) const;

	// Serialize to/from XML file
	virtual Bool SerializeXML( IFile& file, void* data ) const;

	// Returns true if property should be walked by GC
	virtual Bool IsGCRelevant() const;

	// Get type name
	virtual CName GetTypeName() const;

	//! Check type match, used to check if script type definition matches native one
	virtual Bool CompareType( IField* field, String& error ) const;


public:
	// Get number of elements in array
	Uint GetArraySize( const void* arrayData ) const;

	// Get offset to n-th element
	void* GetArrayElement( void* arrayData, Uint index ) const;

	// Get offset to n-th element
	const void* GetArrayElement( const void* arrayData, Uint index ) const;

	// Add element to array
	Int AddArrayElement( void* arrayData, Uint count=1 ) const;

	// Delete single array element
	Bool DeleteArrayElement( void* arrayData, Int itemIndex ) const;

	// Insert item
	Bool InsertArrayElementAt( void* arrayData, Int itemIndex ) const;
};

// Convert property data buffer value to variant value
FORCE_INLINE CVariant CPropertyDataBuffer::ToVariant() const
{
	if ( m_property && Size() )
	{
		// Create variant value
		return CVariant( m_property->GetTypeName(), TypedData() );
	}
	else
	{
		// No value
		return CVariant();
	}
}
