/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dependencyFileTables.h"

/// Dependency file unpacker - allows high level access to dependency file
class CDependencyFileUnpacker
{
public:
	CDependencyFileUnpacker();
	~CDependencyFileUnpacker();

	// Load data from file, can load from any offset
	Bool Load( IFile& sourceFile );

	// Save data to file, can append at given offset
	Bool Save( IFile& targetFile ) const;

	// Get file tables
	RED_FORCE_INLINE CDependencyFileData& GetTables() { return m_tables; }
	RED_FORCE_INLINE const CDependencyFileData& GetTables() const { return m_tables; }

protected:
	// loaded tables
	CDependencyFileData	m_tables;

	// file version
	Uint32	m_version;

	// data for loaded stuff
	struct LoadedData
	{
		TDynArray< Uint8, MC_Temporary >		m_data;
	};

	// loaded objects and buffers
	typedef TDynArray< LoadedData, MC_Temporary > TLoadedData;
	TLoadedData		m_loadedObjects;
	TLoadedData		m_loadedBuffers;

	// extension callback
	virtual void OnModifyTables( CDependencyFileData& fileTables, const Uint64 headerOffset ) const {};
	virtual void OnSaveAdditionalData( IFile& targetFile, CDependencyFileData& fileTables, const Uint64 headerOffset ) const {};
};