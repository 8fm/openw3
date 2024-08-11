/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "bitField.h"
#include "math.h"

CBitField::CBitField( const CName& name, Uint32 size )
	: m_name( name )
	, m_options()
	, m_size( size )
	, m_usedBitMask( 0 )
{
	if ( size != 1 && size != 2 && size != 4 )
	{
		HALT( "Bifield can be only 1,2 or 4 bytes in size!!!" );
		return;
	}
}

CName CBitField::GetBitName( Uint32 bitIndex ) const
{
	ASSERT( bitIndex < ARRAY_COUNT( m_options ) );
	return m_options[ bitIndex ];
}

void CBitField::AddBit( const CName& name, Uint32 bitMask )
{
	// Make sure bit value is OK
	if ( !IsPow2( (Int32)bitMask ) )
	{
		HALT( "Bit filed value '%ls' in '%ls' is not a valid bit", name.AsString().AsChar(), m_name.AsString().AsChar() );
		return;
	}

	// Set the bit name
	Uint32 bitIndex = 0;
	while ( bitMask > 1 )
	{
		bitMask /= 2;
		bitIndex++;
	}

	// Make sure it's not duplicated
	if ( m_options[ bitIndex ] )
	{
		HALT( "Bit filed value '%ls' in '%ls' alredy specified as '%ls'", name.AsString().AsChar(), m_name.AsString().AsChar(), m_options[ bitIndex ].AsString().AsChar() );
		return;
	}

	// Set the bit name	
	m_usedBitMask |= 1 << bitIndex;
	m_options[ bitIndex ] = name;
}

void CBitField::Construct( void* ) const
{
	// nothing here
}

void CBitField::Destruct( void* ) const
{
	// nothing here
}

void CBitField::Get( const void *object, void *propertyData )
{
	switch ( GetSize() )
	{
		case 1: *( Int8* )propertyData = *( const Int8 *) object; break;
		case 2: *( Int16* )propertyData = *( const Int16 *) object; break;
		case 4: *( Int32* )propertyData = *( const Int32 *) object; break;
		default: HALT( "Invalid bitfield size" ); break;
	}	
}

void CBitField::Set( void *object, const void *propertyData )
{
	switch ( GetSize() )
	{
		case 1: *( Int8* )object = *( const Int8 *) propertyData; break;
		case 2: *( Int16* )object = *( const Int16 *) propertyData; break;
		case 4: *( Int32* )object = *( const Int32 *) propertyData; break;
		default: HALT( "Invalid bitfield size" ); break;
	}	
}

Bool CBitField::Compare( const void* data1, const void* data2, Uint32 ) const
{
	switch ( GetSize() )
	{
		case 1: return *( const Int8* )data1 == *( const Int8 *) data2; break;
		case 2: return *( const Int16* )data1 == *( const Int16 *) data2; break;
		case 4: return *( const Int32* )data1 == *( const Int32 *) data2; break;
		default: HALT( "Invalid bitfield size" ); break;
	}	

	return false;
}

void CBitField::Copy( void* dest, const void* src ) const
{
	switch ( GetSize() )
	{
		case 1: *( Int8* )dest = *( const Int8 *) src; break;
		case 2: *( Int16* )dest = *( const Int16 *) src; break;
		case 4: *( Int32* )dest = *( const Int32 *) src; break;
		default: HALT( "Invalid bitfield size" ); break;
	}	
}

void CBitField::Clean( void* ) const
{
	// nothing here
}

Bool CBitField::Serialize( IFile& file, void* data ) const
{
	if ( file.IsReader() )
	{
		Uint32 bitValue = 0;

		// Load set bits
		for ( ;; )
		{
			// Load bit name
			CName bitName;
			file << bitName;

			// End of list
			if ( !bitName )
			{
				break;
			}

			// Set the bit
			for ( Uint32 i=0; i<ARRAY_COUNT( m_options ); i++ )
			{
				if ( m_options[i] == bitName )
				{
					bitValue |= ( 1 << i );
					break;
				}
			}
		}

		// Set value
		//platform independent code
		switch ( GetSize() )
		{
			case 1:
			{
				Uint8 val = (Uint8)bitValue;					
				Copy( data, &val );
				break;
			}

			case 2:
			{
				Uint16 val = (Uint16)bitValue;					
				Copy( data, &val );
				break;
			}

			case 4:
			{
				Copy( data, &bitValue );
				break;
			}

			default:
			{
				HALT( "Invalid bitfield size" );
				break;
			}
		}
	}
	else if ( file.IsWriter() )
	{
		// Read value
		Uint32 bitValue = 0;
		Copy( &bitValue, data );

		// Save bits
		for ( Uint32 i=0; i<ARRAY_COUNT( m_options ); i++ )
		{
			const Uint32 bitMask = 1 << i;
			if ( (bitValue & bitMask) && m_options[i] )
			{
				// Save the bit name
				CName bitName = m_options[i];
				file << bitName;
			}
		}

		// End of array
		CName endOfList = CName::NONE;
		file << endOfList;
	}

	// Saved
	return true;
}

Bool CBitField::ToString( const void* object, String& valueString ) const
{
	// Read value
	Uint32 bitValue = 0;
	Copy( &bitValue, object );

	// Save bits
	String bitOptions;
	for ( Uint32 i=0; i<ARRAY_COUNT( m_options ); i++ )
	{
		const Uint32 bitMask = 1 << i;
		if ( (bitValue & bitMask) && m_options[i] )
		{
			// Save the bit name
			if ( !bitOptions.Empty() ) bitOptions += TXT(";");
			bitOptions += m_options[i].AsString().AsChar();
		}
	}

	// Output is always OK
	valueString = bitOptions;
	return true;
}

Bool CBitField::FromString( void* object, const String& valueString ) const
{
	// Split to bit names
	TDynArray< String > bitNames;
	valueString.Slice( bitNames, TXT( ";" ) );

	// Convert bits names to value
	Uint32 bitValue = 0;
	for ( Uint32 i = 0; i < bitNames.Size(); ++i )
	{
		for ( Uint32 j = 0; j < ARRAY_COUNT( m_options ); ++j )
		{
			if ( m_options[ j ] == CName( bitNames[ i ] ) )
			{
				const Uint32 bitMask = 1 << j;
				bitValue |= bitMask;
				break;
			}
		}
	}

	// Save value
	Copy( object, &bitValue );
	return true;
}
