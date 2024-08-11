/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dependencyFileTables.h"
#include "dependencyFileUnpacker.h"

CDependencyFileUnpacker::CDependencyFileUnpacker()
	: m_version( VER_CURRENT )
{
}

CDependencyFileUnpacker::~CDependencyFileUnpacker()
{
}

Bool CDependencyFileUnpacker::Load( IFile& sourceFile )
{
	// header offset
	const Uint64 baseOffset = sourceFile.GetOffset();

	// load tables
	if ( !m_tables.Load( sourceFile, m_version ) )
		return false;

	// load objects
	m_loadedObjects.Resize( m_tables.m_exports.Size() );
	for ( Uint32 i=0; i<m_tables.m_exports.Size(); ++i )
	{
		const auto& exp = m_tables.m_exports[i];
		auto& data = m_loadedObjects[i].m_data;

		data.Resize( exp.m_dataSize );
		sourceFile.Seek( baseOffset + exp.m_dataOffset );
		sourceFile.Serialize( data.TypedData(), data.Size() );
	}

	// load buffers
	m_loadedBuffers.Resize( m_tables.m_exports.Size() );
	for ( Uint32 i=0; i<m_tables.m_buffers.Size(); ++i )
	{
		const auto& buf = m_tables.m_buffers[i];
		auto& data = m_loadedBuffers[i].m_data;

		if ( buf.m_dataOffset > 0 )
		{
			data.Resize( buf.m_dataSizeOnDisk);
			sourceFile.Seek( baseOffset + buf.m_dataOffset );
			sourceFile.Serialize( data.TypedData(), data.Size() );
		}
	}

	// loaded
	return true;
}

Bool CDependencyFileUnpacker::Save( IFile& targetFile ) const
{
	// header offset
	const Uint64 headerOffset = targetFile.GetOffset();

	// write tables
	CDependencyFileData tablesToSave( m_tables );
	OnModifyTables( tablesToSave, headerOffset );

	// save the tables
	tablesToSave.Save( targetFile, headerOffset );

	// save objects
	for ( Uint32 i=0; i<tablesToSave.m_exports.Size(); ++i )
	{
		auto& exp = tablesToSave.m_exports[i];
		const auto& data = m_loadedObjects[i].m_data;

		exp.m_dataOffset = (Uint32)( targetFile.GetOffset() - headerOffset );
		exp.m_dataSize = data.Size();

		targetFile.Serialize( (void*) data.TypedData(), data.Size() );
	}

	// save buffers
	for ( Uint32 i=0; i<tablesToSave.m_buffers.Size(); ++i )
	{
		auto& buf = tablesToSave.m_buffers[i];
		const auto& data = m_loadedBuffers[i].m_data;

		if ( buf.m_dataOffset > 0 )
		{
			buf.m_dataOffset = (Uint32)( targetFile.GetOffset() - headerOffset );
			buf.m_dataSizeOnDisk = data.Size();

			targetFile.Serialize( (void*) data.TypedData(), data.Size() );
		}
	}

	// save additional data to file at the end of the file
	OnSaveAdditionalData( targetFile, tablesToSave, headerOffset );

	// update the tables
	const Uint64 currentOffset = targetFile.GetOffset();
	targetFile.Seek( headerOffset );
	tablesToSave.Save( targetFile, headerOffset );
	targetFile.Seek( currentOffset );

	// saved
	return true;
}
