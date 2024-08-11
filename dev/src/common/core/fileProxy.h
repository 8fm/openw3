/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "file.h"

/// File proxy - pass transparently all methods to the proper file
/// Can be used to collect statistics and do other shit
class CFileProxy : public IFile
{
public:
	CFileProxy( IFile* file )
		: IFile( file->GetFlags() )
		, m_file( file )
	{
		m_version = file->GetVersion();
	}

	virtual void Serialize( void* buffer, size_t size ) override
	{
		return m_file->Serialize( buffer, size );
	}

	virtual Uint64 GetOffset() const override
	{
		return m_file->GetOffset();
	}

	virtual Uint64 GetSize() const override
	{
		return m_file->GetSize();
	}

	virtual void Seek( Int64 offset ) override
	{
		m_file->Seek( offset );
	}

	virtual class CXMLWriter* QueryXMLWriter()
	{
		return m_file->QueryXMLWriter();
	}

	virtual class CXMLReader* QueryXMLReader()
	{
		return m_file->QueryXMLReader();
	}

	virtual class IDependencyTracker* QueryDepndencyTracker() const
	{
		return m_file->QueryDepndencyTracker();
	}

	virtual class CSpeechCollector* QuerySpeechCollector() const
	{
		return m_file->QuerySpeechCollector();
	}

	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess()
	{
		return m_file->QueryDirectMemoryAccess();
	}

	virtual class IFileLatentLoadingToken* CreateLatentLoadingToken( Uint64 offset )
	{
		return m_file->CreateLatentLoadingToken( offset );
	}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		m_file->SerializePointer( pointerClass, pointer );
	}

	virtual void SerializeName( class CName& name )
	{
		m_file->SerializeName( name );
	}

	virtual void SerializeSoftHandle( class BaseSoftHandle& softHandle )
	{
		m_file->SerializeSoftHandle( softHandle );
	}

	virtual void SerializeTypeReference( class IRTTIType*& type )
	{
		m_file->SerializeTypeReference( type );
	}

	virtual void SerializeDeferredDataBuffer( DeferredDataBuffer & buffer )
	{
		m_file->SerializeDeferredDataBuffer( buffer );
	}

	virtual IFileSkipBlockCache* QuerySkippableBlockCache() 
	{
		return m_file->QuerySkippableBlockCache();
	}

protected:
	IFile*		m_file;
};

/// File proxy that implements IFileEx interface
class CFileProxyEx : public IFileEx
{
public:
	CFileProxyEx( IFile* file )
		: IFileEx( file->GetFlags() )
		, m_file( file )
	{
		m_version = file->GetVersion();
	}

	~CFileProxyEx()
	{
		if( m_file )
		{
			delete m_file;
		}
	}

	virtual void Serialize( void* buffer, size_t size ) override
	{
		return m_file->Serialize( buffer, size );
	}

	virtual Uint64 GetOffset() const override
	{
		return m_file->GetOffset();
	}

	virtual Uint64 GetSize() const override
	{
		return m_file->GetSize();
	}

	virtual void Seek( Int64 offset ) override
	{
		m_file->Seek( offset );
	}

	virtual class CXMLWriter* QueryXMLWriter()
	{
		return m_file->QueryXMLWriter();
	}

	virtual class CXMLReader* QueryXMLReader()
	{
		return m_file->QueryXMLReader();
	}

	virtual class IDependencyTracker* QueryDepndencyTracker() const
	{
		return m_file->QueryDepndencyTracker();
	}

	virtual class CSpeechCollector* QuerySpeechCollector() const
	{
		return m_file->QuerySpeechCollector();
	}

	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess()
	{
		return m_file->QueryDirectMemoryAccess();
	}

	virtual class IFileLatentLoadingToken* CreateLatentLoadingToken( Uint64 offset )
	{
		return m_file->CreateLatentLoadingToken( offset );
	}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		m_file->SerializePointer( pointerClass, pointer );
	}

	virtual void SerializeName( class CName& name )
	{
		m_file->SerializeName( name );
	}

	virtual void SerializeSoftHandle( class BaseSoftHandle& softHandle )
	{
		m_file->SerializeSoftHandle( softHandle );
	}

	virtual void SerializeTypeReference( class IRTTIType*& type )
	{
		m_file->SerializeTypeReference( type );
	}

	virtual void SerializeDeferredDataBuffer( DeferredDataBuffer & buffer )
	{
		m_file->SerializeDeferredDataBuffer( buffer );
	}

	virtual IFileSkipBlockCache* QuerySkippableBlockCache() 
	{
		return m_file->QuerySkippableBlockCache();
	}

	virtual void Close()
	{
		delete m_file;
		m_file = nullptr;
	}

	const void* GetBuffer() const
	{
		return nullptr;
	}

	size_t GetBufferCapacity() const
	{
		return 0;
	}

protected:
	IFile*		m_file;
};