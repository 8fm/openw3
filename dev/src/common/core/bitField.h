/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "rttiType.h"

/// Bitfiled
class CBitField : public IRTTIType
{
protected:
	CName		m_name;
	CName 		m_options[ 32 ];
	Uint32		m_usedBitMask;
	Uint32		m_size;

public:
	CBitField( const CName& name, Uint32 size );

	void AddBit( const CName& name, Uint32 bitMask );

	CName GetBitName( Uint32 bitIndex ) const;

public:
	virtual TMemSize		GetInternalMemSize( const void* ) const { return 0; }

	virtual const CName&	GetName() const { return m_name; }

	virtual Uint32			GetSize() const { return m_size; }

	virtual Uint32			GetAlignment() const { return m_size; } // align to the size of the enum filed

	virtual ERTTITypeType	GetType() const { return RT_BitField; }

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

	virtual Bool			NeedsGC() { return false; }

	template< typename T >
	static String ToString( T value )
	{
		CBitField* field = SRTTI::GetInstance().FindBitField( ::GetTypeName< T >() );
		if ( field )
		{
			String str;
			field->ToString( &value, str );
			return str;
		}

		return RED_NAME( Unknown ).AsString();
	}
};
