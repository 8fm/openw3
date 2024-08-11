/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

// HELL NO!
// You CANNOT depend on r4 here!
//#include "../../games/r4/worldMap.h"

#if 0

class CWorldMapImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CWorldMapImporter, IImporter, 0 );

public:
	CWorldMapImporter();
	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CWorldMapImporter )
	PARENT_CLASS( IImporter )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CWorldMapImporter );

CWorldMapImporter::CWorldMapImporter()
{
	m_resourceClass = ClassID< CWorldMap >();
	m_formats.PushBack( CFileFormat( TXT("csv"), TXT("Worldmap data file") ) );
}

CResource* CWorldMapImporter::DoImport( const ImportOptions& options )
{
	// Factory info
	CWorldMap::FactoryInfo info;
	info.m_parent = options.m_parentObject;
	info.m_reuse = Cast< CWorldMap >( options.m_existingResource );
	info.m_params = options.m_params ? static_cast< WorldMapImporterParams* >( options.m_params ) : NULL;

	// Parse worldmap definition file
	C2dArray* defArr = C2dArray::CreateFromString( options.m_sourceFilePath );
	if ( !defArr )
	{
		WARN_IMPORTER( TXT("Unable to load world map template definition from file %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}

	// Get columns index
	Int32 colTag = defArr->GetColumnIndex( TXT("tag") );
	Int32 colPosX = defArr->GetColumnIndex( TXT("posX") );
	Int32 colPosY = defArr->GetColumnIndex( TXT("posY") );
	Int32 colType = defArr->GetColumnIndex( TXT("type") );
	Int32 colComment = defArr->GetColumnIndex( TXT("comment") );

	// Check index
	if ( colPosX == -1 )
	{
		defArr->Discard();
		WARN_IMPORTER( TXT("Unable to find worldmap's column 'posX' in worldmap definition %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}
	if ( colPosY == -1 )
	{
		defArr->Discard();
		WARN_IMPORTER( TXT("Unable to find worldmap's column 'posY' in worldmap definition %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}
	if ( colTag == -1 )
	{
		defArr->Discard();
		WARN_IMPORTER( TXT("Unable to find worldmap's column 'tag' in worldmap definition %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}
	if ( colType == -1 )
	{
		defArr->Discard();
		WARN_IMPORTER( TXT("Unable to find worldmap's column 'type' in worldmap definition %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}
	if ( colComment == -1 )
	{
		defArr->Discard();
		WARN_IMPORTER( TXT("Unable to find worldmap's column 'comment' in worldmap definition %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}

	// Get row num
	Uint32 rows = static_cast< Uint32 >( defArr->GetNumberOfRows() );
	if ( rows == 0 )
	{
		defArr->Discard();
		return NULL;
	}

	// Parse import file
	for ( Uint32 i = 0; i < rows; i++ )
	{
		CWorldMap::FactoryInfo::StaticMapPinImportData data;

		data.m_tag = CName( defArr->GetValue( colTag, i ) );
		::FromString( defArr->GetValue( colPosX, i ), data.m_posX );
		::FromString( defArr->GetValue( colPosY, i ), data.m_posY );
		data.m_type = CName( defArr->GetValue( colType, i ) );
		data.m_comment = defArr->GetValue( colComment, i );
		info.m_importData.PushBack( data );
	}

	// Discard definition array
	defArr->Discard();

	// Create and return
	CWorldMap* retVal = CWorldMap::Create( info );
	return retVal;
}
#endif