/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/depot.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibConst.h"

class CSimpleBufferWriter;

namespace PathLib
{

class CAreaDescription;


class CAreaRes
{
private:
	template < class T, class Fun >
	static Bool InternalLoad( T* me, const String& depotPath, const Fun& fun );
protected:
	template < class T >
	static Bool Save( const T* me, const String& depotPath );
	template < class T >
	static Bool Load( T* me, const String& depotPath );
	template < class T >
	static Bool Load( T* me, const String& depotPath, CAreaDescription* area );
public:
	CAreaRes()																{}
	virtual ~CAreaRes()														{}

	virtual Bool				VHasChanged() const;
	virtual Bool				VSave( const String& depotPath ) const;

	virtual void				VOnPreLoad( CAreaDescription* area );								// synchronous pre-loading
	virtual Bool				VLoad( const String& depotPath, CAreaDescription* area );			// asynchronous loading
	virtual void				VOnPostLoad( CAreaDescription* area );								// asynchronous possibly heavy processing done on pathlib task (insteady of loading thread)

	virtual void				VOnPreUnload( CAreaDescription* area );

	virtual const Char*			VGetFileExtension() const					= 0;
	virtual ENavResType			VGetResType() const 						= 0;

	static const Char*			GetFileExtension()							{ return TXT("Plz reimplement me!"); }
	static ENavResType			GetResType()								{ ASSERT( TXT("PLZ reimplement me!") ); return NavRes_Invalid; }
};

template < class T >
Bool CAreaRes::Save( const T* me, const String& depotPath )
{
	IFile* fileWriter = GFileManager->CreateFileWriter( depotPath );
	if( !fileWriter )
	{
		return false;
	}

	TDynArray< Int8 > buffer;
	CSimpleBufferWriter writer( buffer, T::RES_VERSION );

	me->WriteToBuffer( writer );
	
	fileWriter->Serialize( buffer.Data(), buffer.DataSize() );
	delete fileWriter;

	return true;
}

template < class T, class Fun >
Bool CAreaRes::InternalLoad( T* me, const String& depotPath, const Fun& fun )
{
	// open file
	CDiskFile* diskFile = GDepot->FindFile( depotPath );
	IFile* fileReader = diskFile ? diskFile->CreateReader() : nullptr;
	if ( !fileReader )
	{
		return false;
	}
	// read data
	Bool bufferIsAllocated;
	Uint32 fileSize;
	Int8* buffer;
	if ( IFileDirectMemoryAccess* directMemory = fileReader->QueryDirectMemoryAccess() )
	{
		// use already allocated data from buffer
		bufferIsAllocated = false;
		fileSize = directMemory->GetBufferSize();
		buffer = reinterpret_cast< Int8* >( directMemory->GetBufferBase() );
		
	}
	else
	{
		bufferIsAllocated = true;
		fileSize = Uint32( fileReader->GetSize() );			// we don't support uint64 navigation files
		buffer = static_cast< Int8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_PathLib, fileSize ) );
		fileReader->Serialize( buffer, fileSize );
	}

	CSimpleBufferReader reader( buffer, fileSize );

	Bool result = fun( me, reader );
	delete fileReader;

	if ( bufferIsAllocated )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_PathLib, buffer );
	}

	return result;
}

template < class T >
Bool CAreaRes::Load( T* me, const String& depotPath )
{
	auto fun =
		[] ( T* me, CSimpleBufferReader& reader ) -> Bool
		{
			 return me->ReadFromBuffer( reader );
		};

	return InternalLoad( me, depotPath, fun );
}

template < class T >
Bool CAreaRes::Load( T* me, const String& depotPath, CAreaDescription* area )
{
	auto fun =
		[ area ] ( T* me, CSimpleBufferReader& reader ) -> Bool
		{
			return me->ReadFromBuffer( reader, area );
		};

	return InternalLoad( me, depotPath, fun );
}


};			// namespace PathLib
