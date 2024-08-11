/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "rttiType.h"
#include "namesPool.h"
#include "namesRegistry.h"


class CEnum : public IRTTIType
{
public:
	typedef Int32 TValueType;

protected:
	CName					m_name;
	TDynArray< CName >		m_options;
	THashMap< CName, Int32 >	m_values;
	THashMap< Int32, CName >	m_names;
	Uint32					m_size;
	Bool					m_isScripted;

public:
	CEnum( const CName& name, Uint32 size, Bool scripted );

	void ReuseScriptStub( Uint32 size );

	void Add( const CName& name, Int32 value );

	void Remove( const CName& name );

	Bool FindValue( const CName& name, Int32 &value ) const;

	Bool FindName( const Int32 value, CName& name ) const;

	const TDynArray< CName >& GetOptions() const { return m_options; }

	Bool IsScripted() const { return m_isScripted; }

	Int32 GetAsInt( const void *object ) const;

	void SetAsInt( void *object, Int32 data ) const;

	void SerializeSimple( IFile& file, void* data ) const;

public:
	virtual TMemSize		GetInternalMemSize( const void* ) const { return 0; }

	virtual const CName&	GetName() const { return m_name; }

	virtual Uint32			GetSize() const { return m_size; }

	virtual Uint32			GetAlignment() const { return m_size; } // align to the size of the enum filed

	virtual ERTTITypeType	GetType() const { return RT_Enum; }

	virtual void			Construct( void *mem ) const;

	virtual void			Destruct( void *mem ) const;

	virtual void 			Get( const void *object, void *propertyData );

	virtual void 			Set( void *object, const void *propertyData );

	virtual Bool 			Compare( const void* data1, const void* data2, Uint32 flags ) const;

	virtual void 			Copy( void* dest, const void* src ) const;

	virtual void 			Clean( void* data ) const;

	virtual Bool 			Serialize( IFile& file, void* data ) const;

	virtual Bool 			ToString( const void* object, String& valueString ) const;

	virtual Bool 			FromString( void* object, const String& valueString ) const;

	virtual Bool 			NeedsGC() { return false; }

public:
	template< typename T >
	static String ToString( T value )
	{
		CEnum* field = SRTTI::GetInstance().FindEnum( ::GetTypeName< T >() );
		if ( field )
		{
			CName name;
			if ( field->FindName( value, name ) )
			{
				return name.AsString();
			}
		}

		return CNAME( Unknown ).AsString();
	}

	template< typename T >
	static Float ToFloat( const T& value )
	{
		// This method was created in a response to a most peculiar bug:
		// In specific cases simple conversion e.g. (Float) ((Int32) E_EnumValue) was giving wrong results (like 0.0f instead of 1.0f)
		// This method is crude and inefficient but it gives proper results
		// Do not use this method unless you encounter the same kind of problem
		CEnum* field = SRTTI::GetInstance().FindEnum( ::GetTypeName< T >() );
		if ( field )
		{
			CName fieldName;
			Int32 fieldValue;
			if ( field->FindName( value, fieldName ) )
			{
				if ( field->FindValue( fieldName, fieldValue ) )
				{
					return (Float) fieldValue;
				}
			}
		}

		HALT( "Converting invalid enum value to float!" );
		return FLT_MAX;
	}
};
