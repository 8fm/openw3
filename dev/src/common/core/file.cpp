/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "file.h"
#include "names.h"
#include "string.h"
#include "dependencyLinker.h"
#include "softHandle.h"
#include "math.h"
#include "object.h"
#include "resourceid.h"
#include "deferredDataBuffer.h"
#include "namesReporter.h"
#include "memoryHelpers.h"

Bool GCNameAsNumberSerialization( false );

void IFile::SerializePointer( const class CClass* pointerClass, void*& pointer )
{
	// nothing happens in normal file
	RED_UNUSED( pointerClass );
	RED_UNUSED( pointer );
}

void IFile::SerializeName( class CName& name )
{
	// Save using hash
	if ( !IsGarbageCollector() )
	{
		if ( IsWriter() )
		{
			Bool writeHash( IsHashNames() );
			Bool writeString( !writeHash );

			if ( !GCNameAsNumberSerialization )
			{
				// The game is not cooked, and we're not cooking cnames
				writeString = true;
			}

			if ( writeHash )
			{
				// Write hash 
				Uint32 hash = writeString ? 0 : name.GetSerializationHash().GetValue();
				*this << hash;
			}
			
			if ( writeString )
			{
				// CName is already converted to Ansi, so I skip the TString<>::Serialize() code here to save us some memory allocations 
				// and detecting if it can be saved as Ansi or not (sure it can, it's converted already).

				const AnsiChar* buf = name.AsAnsiChar();
				const size_t byteCount = Red::StringLength( buf );
				Int32 byteCountNegative = -Int32( byteCount ); 

				*this << CCompressedNumSerializer( byteCountNegative );
				Serialize( ( void* ) buf, byteCount * sizeof( AnsiChar ) );
			}

		}
		else if ( IsReader() )
		{
			Bool readHash( IsHashNames() );
			Bool readString( true );
			Uint32 hash( 0 );

			if ( readHash )
			{
				*this << hash;

				// Special hash token == 0 means we should fall back to reading a string
				if ( hash != 0 )
				{
					readString = false;
				}
			}

			if ( readString )
			{
				StringBuffer< 512 > uniName;
				*this << uniName;

				// Initialize from string
				name = CName( uniName.AsChar() );
			}
			else
			{
				// Initialize from hash 
				name = CName::CreateFromHash( Red::CNameHash( hash ) );
			}
		}
	}
}

void IFile::SerializeNameAsHash( class CName& name )
{
	// Save using hash - explicitly
	if ( IsWriter() )
	{
		Uint32 hash( name.GetSerializationHash().GetValue() );
		ASSERT( 0 != hash );

		*this << hash;
	}
	else
	{
		Uint32 hash;
		*this << hash;

		ASSERT( 0 != hash );
		name = CName::CreateFromHash( Red::CNameHash( hash ) );
	}
}

void IFile::SerializeSoftHandle( class BaseSoftHandle& softHandle )
{
	if ( IsWriter() )
	{
		String path( softHandle.GetPath() );
		*this << path;
	}
	else if ( IsReader() )
	{
		String path;
		*this << path;
		softHandle = BaseSoftHandle( path );
	}
}

void IFile::SerializeTypeReference( class IRTTIType*& type )
{
	if ( !IsGarbageCollector() )
	{
		if ( IsReader() )
		{
			// load type name
			CName typeName;
			SerializeName( typeName );

			// find type
			type = SRTTI::GetInstance().FindType( typeName );
		}
		else if ( IsWriter() )
		{
			// save type name
			CName typeName = type ? type->GetName() : CName();
			SerializeName( typeName );
		}
	}
}

void IFile::SerializePropertyReference( const class CProperty*& prop )
{
	if ( !IsGarbageCollector() )
	{
		if ( IsReader() )
		{
			// load parent class name
			CName className;
			SerializeName( className );

			// load property name
			CName propName;
			SerializeName( propName );

			// find property in given class
			prop = nullptr;
			if ( className && propName )
			{
				CClass* classType = SRTTI::GetInstance().FindClass( className );
				if ( classType )
				{
					prop = classType->FindProperty( propName );
				}
			}
		}
		else if ( IsWriter() )		
		{
			CName className;
			CName propName;
			if ( prop && prop->GetParent() )
			{
				className = prop->GetParent()->GetName();
				propName = prop->GetName();
			}

			// save type name
			SerializeName( className );
			SerializeName( propName );
		}
	}
}

void IFile::SerializeDeferredDataBuffer( DeferredDataBuffer & buffer )
{
	buffer.SerializeDirectlyAsRawBuffer( *this );
	//buffer.Serialize( *this );
}

void IFile::CopyToFile( IFile &dstFile, Uint64 offset, Uint64 size )
{
	ASSERT( IsReader() && dstFile.IsWriter() );

	Uint64 endPosition = offset + size;
	ASSERT( endPosition <= GetSize() );

	const Uint64 BUFFER_SIZE = 16 * 1024;
	Uint8 buffer[ BUFFER_SIZE ];

	Seek( offset );

	for ( Uint64 position = offset; position < endPosition; position += BUFFER_SIZE ) 
	{
		Uint64 sizeToRead = BUFFER_SIZE;
		if ( position + sizeToRead > endPosition ) 
		{
			sizeToRead = endPosition - position;
		}
		Serialize( buffer, sizeToRead );
		dstFile.Serialize( buffer, sizeToRead );
	}
}

IFile& IFile::operator<<( class CObject*& object )
{
	SerializePointer( ClassID< CObject >(), (void*&)object );
	return *this;
}

IFile& operator<<( IFile& file, Vector& val )
{
	return file << val.A[0] << val.A[1] << val.A[2] << val.A[3];
}

IFile& operator<<( IFile& file, Vector3& val )
{
	return file << val.A[0] << val.A[1] << val.A[2];
}

IFile& operator<<( IFile& file, Vector2& val )
{ 
	return file << val.A[0] << val.A[1];
}

IFile& operator<<( IFile& file, Color& val )
{ 
	return file << val.R << val.G << val.B << val.A;
}

IFile& operator<<( IFile& file, EulerAngles& val )
{ 
	return file << val.Roll << val.Pitch << val.Yaw;
}

IFile& operator<<( IFile& file, Matrix& val )
{
	return file << val.V[0] << val.V[1] << val.V[2] << val.V[3];
}

IFile& operator<<( IFile& file, Plane& val )
{
	return file << val.NormalDistance;
	}

IFile& operator<<( IFile& file, CGUID& guid )
{
	return file << guid.parts.A << guid.parts.B << guid.parts.C << guid.parts.D;
}

IFile& operator<<( IFile& file, Red::Core::ResourceManagement::CResourceId& val )
{
	for( Uint32 i = 0; i < Red::Core::ResourceManagement::CResourceId::NUM_PARTS_64; ++i )
	{
		file << val[ i ];
	}

	return file;
}

IFile& operator<<( IFile& file, DeferredDataBuffer& buffer )
{
	file.SerializeDeferredDataBuffer( buffer );
	return file;
}

void* IFile::operator new( size_t size )			
{ 
	return RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_Engine, size );
}		

void IFile::operator delete( void* ptr ) 
{ 
	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_Engine, ptr );
}	



CFileDirectSerializationTables::CFileDirectSerializationTables()
{
	m_mappedNames = nullptr;
	m_numMappedNames = 0;
	m_mappedTypes = nullptr;
	m_numMappedTypes = 0;
	m_mappedProperties = 0;
	m_mappedPropertyOffsets = 0;
	m_numMappedProperties = 0;
}

void ISaveFile::SerializeName( class CName& name )
{
	// Save using remapper
	if ( IsWriter() )
	{
		Uint16 v = ( Uint16 ) m_remapper.Map( name );
		*this << v;
	}
	else if ( IsReader() )
	{
		if ( m_saveVersion < 27 ) // shitty hack, but to be removed very soon either way
		{
			// this pile of code here is for supporting old versions :(

			Bool readHash( IsHashNames() );
			Bool readString( true );
			Uint32 hash( 0 );

			if ( readHash )
			{
				*this << hash;

				if ( IsKeepingCNameList() || hash != 0 )
				{
					// Special hash token == 0 means we should fall back to reading a string
					readString = false;
				}
			}

			if ( readString )
			{
				StringBuffer< 512 > uniName;
				*this << uniName;

				// Initialize from string
				name = CName( uniName.AsChar() );
			}
			else
			{
				// Initialize from hash 
				name = CName::CreateFromHash( Red::CNameHash( hash ) );
			}
		}
		else
		{
			Uint16 v;
			*this << v;
			name = m_remapper.Map( v );
		}
	}
}
