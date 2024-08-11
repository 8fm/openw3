/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "latentDataBuffer.h"
#include "memoryFileWriter.h"
#include "memoryFileReader.h"
#include "fileLatentLoadingToken.h"

template< typename T >
void DeferredDataBuffer::WriteObject( T& inputObject )
{
	TDynArray< Uint8 > result;
	CMemoryFileWriter memoryFileWriter( result );
	memoryFileWriter << inputObject;

	this->SetBufferContent( result.Data(), (Uint32) result.DataSize() );
}

template< typename T >
Red::TUniquePtr< T > DeferredDataBuffer::ReadObject( const Uint32 fileVersion ) const
{
	Red::TUniquePtr< T > value( new T );

	BufferHandle data = this->AcquireBufferHandleSync();
	if ( data )
	{
		CMemoryFileReaderExternalBuffer memoryFile( data->GetData(), data->GetSize() );
		memoryFile.m_version = fileVersion;
		memoryFile << *value;
	}
	
	return value;
}

RED_INLINE Uint32 DeferredDataBuffer::GetSize() const
{
	return m_size;
}


