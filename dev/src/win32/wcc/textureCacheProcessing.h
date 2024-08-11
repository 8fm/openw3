/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "../../common/engine/textureCache.h"
#include "../../common/core/dataBuffer.h"

//////////////////////////////////////////////////////////////////////////
// Utilities for processing existing texture cache files


class CTextureCacheData
{
	friend class CTextureCacheDataSaver;

private:
	IFile*							m_inputFile;

	TextureCacheHeader				m_header;

	TDynArray< TextureCacheEntry >	m_entries;
	TDynArray< AnsiChar >			m_strings;
	TDynArray< Uint32 >				m_mipOffsets;

	String							m_sourceFilePath;

public:
	CTextureCacheData();
	~CTextureCacheData();

	Bool LoadFromFile( const String& absolutePath );
	const String& GetSourceFilePath() const { return m_sourceFilePath; }

	Uint32 GetNumEntries() const { return m_entries.Size(); }

	const TextureCacheEntry* GetEntry( Uint32 index ) const;

	String GetEntryName( Uint32 index ) const;
	Uint32 GetEntryDiskSize( Uint32 index ) const;
	Uint32 GetEntryDiskSizeUnpadded( Uint32 index ) const;
	Uint32 GetEntryHash( Uint32 index ) const;

	// Returns an IFile suitable for reading from the given entry. The file's size will match the entry's
	// unpadded disk size. Returned IFile should be deleted by caller. CTextureCacheData must not be
	// destroyed while the returned IFile is being used. Access to multiple entries at the same time is
	// not supported (should delete this IFile before Resuming another entry).
	IFile* ResumeEntryData( Uint32 index );
};


class CTextureCacheDataSaver
{
private:
	IFile*							m_outputFile;

	TextureCacheHeader				m_header;

	TDynArray< TextureCacheEntry >	m_entries;
	TDynArray< AnsiChar >			m_strings;
	TDynArray< Uint32 >				m_mipOffsets;

	TDynArray< Uint8 >				m_readBuffer;

	THashMap< Uint32, Uint32 >		m_hashToIndex;					// Track the entry hashes that have been added, to detect duplicates

public:
	CTextureCacheDataSaver( const String& absolutePath );
	~CTextureCacheDataSaver();

	operator Bool() const { return m_outputFile != nullptr; }

	Bool SaveEntry( Uint32 sourceIndex, const CTextureCacheData& sourceData );
};
