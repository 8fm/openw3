
#pragma once

#include "meshCache.h"

enum ECookingPlatform : Int32;

class MeshCacheCooker
{
private:
	TDynArray< MeshCacheEntry >		m_entriesList;

	IFile*							m_file;
	Uint32							m_headerDataOffset;
	Uint32							m_cacheStartOffset;

public:
	MeshCacheCooker( const String& absoluteFileName, ECookingPlatform platform );
	~MeshCacheCooker();

	void AddMesh( Int32& index, DataBuffer* deviceBuffer, String fileName );

	void Save();

private:
	Uint32 SaveEntries();
	Bool LoadFromFile( const String& absoluteFileName );
	void CreateFileHeader( ECookingPlatform platform );
	void CloseFile();
};

extern MeshCacheCooker* GMeshCacheCooker;
extern MeshCacheCooker* GMeshCacheCookerLow;
