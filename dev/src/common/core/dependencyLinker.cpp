/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dependencyLinker.h"

IDependencyLinker::IDependencyLinker( IFile& file )
	: IFile( file.GetFlags() )
	, m_file( &file )
{
	ASSERT( file.IsReader() || (m_version == VER_CURRENT) );
	m_version = file.GetVersion();
}

IDependencyLinker::~IDependencyLinker()
{	
}

void IDependencyLinker::Serialize( void* buffer, size_t size )
{
	m_file->Serialize( buffer, size );
}

Uint64 IDependencyLinker::GetOffset() const
{
	return m_file->GetOffset();
}

Uint64 IDependencyLinker::GetSize() const
{
	return m_file->GetSize();
}

void IDependencyLinker::Seek( Int64 offset )
{
	m_file->Seek( offset );
}

