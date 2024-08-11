/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dependencyFileGluer.h"

CDependencyFileGluer::CDependencyFileGluer()
{
}

Bool CDependencyFileGluer::Load( IFile& sourceFile )
{
	if ( ! CDependencyFileUnpacker::Load( sourceFile ) )
		return false;

	// remove all existing inplace data
	m_tables.m_inplace.Clear();

	// remove the inplace flag from imports
	for ( Uint32 i=0; i<m_tables.m_imports.Size(); ++i )
	{
		m_tables.m_imports[i].m_flags &= ~CDependencyFileData::eImportFlags_Inplace;
	}

	// loaded
	return true;
}

void CDependencyFileGluer::AddFile( const StringAnsi& filePath, const CDependencyFileUnpacker* sourceData )
{
	if ( sourceData != nullptr )
	{
		FileToGlue info;
		info.m_path = filePath;
		info.m_data = sourceData;
		m_filesToGlue.PushBack( info );
	}
}

void CDependencyFileGluer::OnModifyTables( CDependencyFileData& fileTables, const Uint64 headerOffset ) const
{
	// build existing string map
	THashMap< StringAnsi, Uint32 > stringMap;
	BuildStringMap( fileTables, stringMap );

	// unmark all import as inplace
	THashMap< StringAnsi, Uint32 > importMap;
	for ( Uint32 i=0; i<fileTables.m_imports.Size(); ++i )
	{
		auto& imp = fileTables.m_imports[i];
		
		// not an implace imprt
		imp.m_flags &= ~CDependencyFileData::eImportFlags_Inplace;

		// map import in the map
		const AnsiChar* importPath = &fileTables.m_strings[ imp.m_path ];
		importMap.Insert( importPath, i );
	}

	// prepare the inplace list
	fileTables.m_inplace.Clear();
	for ( const auto& info : m_filesToGlue )
	{
		// find existing import info and mark the file as inplace
		Uint32 importIndex = 0;
		if ( importMap.Find( info.m_path, importIndex ) )
		{
			fileTables.m_imports[ importIndex ].m_flags |= CDependencyFileData::eImportFlags_Inplace;
		}

		// add new inplace data definition
		CDependencyFileData::InplaceData data;
		data.m_dataOffset = 0; // for now
		data.m_dataSize = 0;
		data.m_importIndex = importIndex+1;
		data.m_path = MapString( fileTables, stringMap, info.m_path.AsChar() );
		data.m_pathHash = Red::CalculatePathHash64( info.m_path.AsChar() );
		fileTables.m_inplace.PushBack( data );
	}
}

void CDependencyFileGluer::OnSaveAdditionalData( IFile& targetFile, CDependencyFileData& fileTables, const Uint64 headerOffset ) const
{
	// store inplace data
	for ( Uint32 i=0; i<m_filesToGlue.Size(); ++i )
	{
		CDependencyFileData::InplaceData& inplaceData = fileTables.m_inplace[ i ];

		// set file data offset
		const Uint64 startOffset = targetFile.GetOffset();
		inplaceData.m_dataOffset = (Uint32)( startOffset - headerOffset );

		// store the file (recursive)
		m_filesToGlue[i].m_data->Save( targetFile );

		// update the size
		const Uint64 endOffset = targetFile.GetOffset();
		inplaceData.m_dataSize = (Uint32)( endOffset - startOffset );

		/*LOG_CORE( TXT("Glued '%hs' at %d (size %d)"),
			&fileTables.m_strings[ inplaceData.m_path ],
			inplaceData.m_dataOffset, inplaceData.m_dataSize );*/
	}
}

void CDependencyFileGluer::BuildStringMap( const CDependencyFileData& fileTables, THashMap< StringAnsi, Uint32 >& stringMap )
{
	stringMap.Clear();

	// map the strings
	Uint32 pos = 1;
	while ( pos < fileTables.m_strings.Size() )
	{
		const AnsiChar* txt = (const AnsiChar*) &fileTables.m_strings[pos];
		const Uint32 len = (Uint32) Red::StringLength( txt );
		stringMap.Insert( txt, pos );
		pos += (len+1);
	}

}

Uint32 CDependencyFileGluer::MapString( CDependencyFileData& fileTables, THashMap< StringAnsi, Uint32 >& stringMap, const AnsiChar* txt )
{
	// empty string
	if ( !txt || !txt[0] )
		return 0;

	// find in existing table
	Uint32 offset = 0;
	if ( stringMap.Find( txt, offset ) )
		return offset;

	// add to string table
	const Uint32 len = (Uint32) Red::StringLength( txt );
	offset = (Uint32) fileTables.m_strings.Grow( len+1 );
	Red::MemoryCopy( &fileTables.m_strings[offset], txt, sizeof(AnsiChar) * (len+1) );

	// add to map
	stringMap.Insert( txt, offset );
	return offset;
}