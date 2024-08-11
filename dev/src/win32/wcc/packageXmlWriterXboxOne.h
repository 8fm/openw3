/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/utility.h"
#include "packageConstants.h"

class CPackageXmlWriterXboxOne : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	static const Uint32 ALIGNMENT_CHUNK_ID = 0x3FFFFFFF; // Not sure if there's some magic, but this is in MS example, so let's not go over it
	static const Uint32 INVALID_CHUNK_ID = 0xFFFFFFFF;

private:
	struct SFileMapping
	{
		String m_origPath;
		String m_targPath;

		SFileMapping( const String& origPath, const String& targPath )
			: m_origPath( origPath )
			, m_targPath( targPath )
		{}
	};

private:
	IFile&											m_fileWriter;

private:
	THashMap< Uint32, TDynArray< SFileMapping > >	m_chunkIDFilesMap;
	Uint32											m_launchMarkerChunk;

public:
	CPackageXmlWriterXboxOne( IFile& fileWriter );

public:
	Bool ChunkAdd( Uint32 chunkID );
	Bool AddFile( const String& origPath, Uint32 chunkID );
	Bool AddFile( const String& origPath, const String& targPath, Uint32 chunkID );
	Bool SetLaunchMarkerChunk( Uint32 chunkID );

public:
	Bool WriteXml( EPackageType packageType );

public:
	//void MoveFilePathMap( TFilePathMap& outFilePathMap );
};
