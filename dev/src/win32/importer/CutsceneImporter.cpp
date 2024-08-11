/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/importer.h"
#include "../../common/engine/cutscene.h"
#include "../../common/core/2darray.h"

class CCutsceneImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CCutsceneImporter, IImporter, 0 );

public:
	CCutsceneImporter();
	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CCutsceneImporter )
	PARENT_CLASS(IImporter)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS(CCutsceneImporter);

CCutsceneImporter::CCutsceneImporter()
{
	m_resourceClass = ClassID< CCutsceneTemplate >();
	m_formats.PushBack( CFileFormat( TXT("csv"), TXT("Cutscene template file") ) );
}

CResource* CCutsceneImporter::DoImport( const ImportOptions& options )
{
	// Factory info
	CCutsceneTemplate::FactoryInfo info;
	info.m_parent = options.m_parentObject;
	info.m_reuse = Cast< CCutsceneTemplate >( options.m_existingResource );
	info.m_params = options.m_params ? static_cast< CutsceneImporterParams* >( options.m_params ) : NULL;

	// Parse cutscene definition file
	C2dArray* defArr = C2dArray::CreateFromString( options.m_sourceFilePath );
	if ( !defArr )
	{
		WARN_IMPORTER( TXT("Unable to load cutscene template definition from file %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}

	// Get columns index
	Int32 colActors = defArr->GetColumnIndex( TXT("animation") );
	Int32 colComponents = defArr->GetColumnIndex( TXT("component") );

	// Check index
	if ( colActors == -1 )
	{
		defArr->Discard();
		WARN_IMPORTER( TXT("Unable to find actor's column 'animation' in cutscene template definition %s!"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}
	if ( colComponents == -1 )
	{
		defArr->Discard();
		WARN_IMPORTER( TXT("Unable to find component's column 'component' in cutscene template definition %s!"), options.m_sourceFilePath.AsChar() );
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
	for ( Uint32 i=0; i<rows; i++ )
	{
		CCutsceneTemplate::FactoryInfo::ActorImportData data;
		data.m_animation = defArr->GetValue( colActors, i );
		data.m_component = defArr->GetValue( colComponents, i );

		info.m_importData.PushBack( data );
	}

	// Discard definition array
	defArr->Discard();
	
	// Create and return
	CCutsceneTemplate* retVal = CCutsceneTemplate::Create( info );
	return retVal;
}
