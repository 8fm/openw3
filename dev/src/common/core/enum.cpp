/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "enum.h"

CEnum::CEnum( const CName& name, Uint32 size, Bool scripted )
	: m_name( name )
	, m_size( size )
	, m_isScripted( scripted )
{
}

void CEnum::ReuseScriptStub( Uint32 size )
{
	m_size = size;
	m_options.Clear();
	m_values.Clear();
	m_names.Clear();
}

void CEnum::Add( const CName& name, Int32 value )
{
	m_options.PushBack( name );
	m_values.Insert( name, value );
	m_names.Insert( value, name );
}

void CEnum::Remove( const CName& name )
{
	m_options.Erase( m_options.FindPtr( name ) );
	Int32 value = m_values[ name ];
	m_values.Erase( name );
	m_names.Erase( value );
}

Bool CEnum::FindValue( const CName& name, Int32 &value ) const
{
	return m_values.Find( name, value );
}

Bool CEnum::FindName( const Int32 value, CName& name ) const
{
	return m_names.Find( value, name );	
}

void CEnum::Construct( void* ) const
{
	// nothing here
}

void CEnum::Destruct( void* ) const
{
	// nothing here
}

void CEnum::Get( const void *object, void *propertyData )
{
	switch ( GetSize() )
	{
	case 1: 
		*( Int8* )propertyData = *( const Int8 *) object; break;
	case 2: 
		*( Int16* )propertyData = *( const Int16 *) object; break;
	case 4: 
		*( Int32* )propertyData = *( const Int32 *) object; break;
	}	
}

Int32 CEnum::GetAsInt( const void *object ) const
{
	Int32 val = 0;
	switch ( GetSize() )
	{
	case 1: 
		val = *( const Int8 *) object; break;
	case 2: 
		val = *( const Int16 *) object; break;
	case 4: 
		val = *( const Int32 *) object; break;
	}

	return val;
}

void CEnum::SetAsInt( void *object, Int32 data ) const
{
	switch ( GetSize() )
	{
	case 1: 
		*( Int8 *) object = (Int8) data; break;
	case 2: 
		*( Int16 *) object = (Int16)data; break;
	case 4: 
		*( Int32 *) object = data; break;
	}
}

void CEnum::SerializeSimple( IFile& file, void* data ) const
{
	if ( file.IsReader() )
	{
		// Read the string value
		String valueStr;
		file << valueStr;

		// Find the value
		CName valueName( valueStr.AsChar() );
		Int32 value = 0;
		if ( FindValue( valueName, value ) )
		{
			// Emit
			SetAsInt( data, value );
		}
	}
	else if ( file.IsWriter() )
	{
		// Get value
		Int32 val = GetAsInt( data );

		// Get string name
		String valueStr;
		CName valeName;
		if ( FindName( val, valeName ) )
		{
			valueStr = valeName.AsString();
		}

		// Save the string
		file << valueStr;
	}
}

void CEnum::Set( void *object, const void *propertyData )
{
	switch ( GetSize() )
	{
	case 1: 
		*( Int8* )object = *( const Int8 *) propertyData; break;
	case 2: 
		*( Int16* )object = *( const Int16 *) propertyData; break;
	case 4: 
		*( Int32* )object = *( const Int32 *) propertyData; break;
	}	
}

Bool CEnum::Compare( const void* data1, const void* data2, Uint32 ) const
{
	switch ( GetSize() )
	{
	case 1: 
		return *( const Int8* )data1 == *( const Int8 *) data2; break;
	case 2: 
		return *( const Int16* )data1 == *( const Int16 *) data2; break;
	case 4: 
		return *( const Int32* )data1 == *( const Int32 *) data2; break;
	}	

	return false;
}

void CEnum::Copy( void* dest, const void* src ) const
{
	switch ( GetSize() )
	{
	case 1: 
		*( Int8* )dest = *( const Int8 *) src; 
		break;
	case 2: 
		*( Int16* )dest = *( const Int16 *) src; 
		break;
	case 4: 
		*( Int32* )dest = *( const Int32 *) src; 
		break;
	}	
}

void CEnum::Clean( void* ) const
{
	// nothing here
}

Bool CEnum::Serialize( IFile& file, void* data ) const
{
	if ( file.IsReader() )
	{
		Int32 value;
		CName name;
		file << name;

		if ( FindValue( name, value ) )
		{
			switch ( GetSize() )
			{
			case 1: 
				*(Int8*)data = (Int8)value; 
				break;
			case 2: 
				*(Int16*)data = (Int16)value; 
				break;
			case 4: 
				*(Int32*)data = (Int32)value; 
				break;
			}			
		}
	}
	else if ( file.IsWriter() )
	{
		CName name;		
		switch ( GetSize() )
		{
		case 1: 
			FindName( *(Int8*) data, name ); 
			break;
		case 2: 
			FindName( *(Int16*) data, name ); 
			break;
		case 4: 
			FindName( *(Int32*) data, name ); 
			break;
		}		
		file << name;
	}

	return true;
}

Bool CEnum::ToString( const void* object, String& valueString ) const
{
	CName option;
	Uint32 value;

	switch( GetSize() )
	{
	// 1 byte
	case 1:
		value = *static_cast< const Uint8* >( object );
		break;

	// 2 bytes
	case 2:
		value = *static_cast< const Uint16* >( object );
		break;

	// 4 bytes
	case 4:
		value = *static_cast< const Uint32* >( object );
		break;

	// Unhandled case
	default:
		return false;
	}

	if( FindName( value, option ) )
	{
		valueString = option.AsString();
		return true;
	}

	// Name not found
	return false;
}

Bool CEnum::FromString( void* object, const String& valueString ) const
{
	Int32 value = 0;
	if ( FindValue( CName( valueString ), value ) )
	{
		switch ( GetSize() )
		{
		case 1: 
			*(Int8*)object = (Int8)value; 
			break;
		case 2: 
			*(Int16*)object = (Int16)value; 
			break;
		case 4: 
			*(Int32*)object = (Int32)value; 
			break;
		}			
		return true;
	}
	
	return false;
}
