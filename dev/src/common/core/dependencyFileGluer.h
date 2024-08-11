/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dependencyFileUnpacker.h"

/// Helper class for gluing multiple files together
class CDependencyFileGluer : public CDependencyFileUnpacker
{
public:
	CDependencyFileGluer();

	/// Load content
	Bool Load( IFile& sourceFile );

	/// Add additional file to glue
	void AddFile( const StringAnsi& filePath, const CDependencyFileUnpacker* sourceData );

protected:
	struct FileToGlue
	{
		StringAnsi							m_path;
		const CDependencyFileUnpacker*		m_data;
	};

	// Files to glue
	typedef TDynArray< FileToGlue >		TFilesToGlue;
	TFilesToGlue			m_filesToGlue;

	/// Extend file
	virtual void OnModifyTables( CDependencyFileData& fileTables, const Uint64 headerOffset ) const override;
	virtual void OnSaveAdditionalData( IFile& targetFile, CDependencyFileData& fileTables, const Uint64 headerOffset ) const override;

	/// Helper stuff
	static Uint32 MapString( CDependencyFileData& fileTables, THashMap< StringAnsi, Uint32 >& stringMap, const AnsiChar* txt );
	static void BuildStringMap( const CDependencyFileData& fileTables, THashMap< StringAnsi, Uint32 >& stringMap );
};