/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameplayEntityLocalizationExporter.h"

#include "../../../common/game/gameplayEntity.h"
#include "../../../common/core/depot.h"

void CGameplayEntityLocalizationExporter::BeginBatchExport()
{
	m_exportData.Clear();

	TDynArray< String > headers;
	headers.PushBack( TXT( "Source File" ) );
	headers.PushBack( TXT( "StringID" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "Class" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "Text SL" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );

	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		headers.PushBack( TXT( "" ) );
		headers.PushBack( TXT( "" ) );
		headers.PushBack( TXT( "" ) );
		headers.PushBack( TXT( "Text TL" ) );
		headers.PushBack( TXT( "" ) );
		headers.PushBack( TXT( "" ) );
	}

	headers.PushBack( TXT( "" ) );

	m_exportData.PushBack( headers );

	PrepareErrorFileHeaders();
}

void CGameplayEntityLocalizationExporter::EndBatchExport()
{
	String exportString = ConvertToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFileWithUTF8( m_savePath, exportString );
	}

	DumpErrorsFile();
}

void CGameplayEntityLocalizationExporter::ExportResource( CResource* resource )
{
	CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( resource );
	CGameplayEntity* entity = Cast< CGameplayEntity >( entityTemplate->GetEntityObject() );
	
	TDynArray< String > elementData;
	elementData.PushBack( GetResourceName( entityTemplate ) );
	elementData.PushBack( ToString( entity->GetLocalizedDisplayName().GetIndex() ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( entity->GetClass()->GetName().AsString() );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( GetSourceString( entity->GetLocalizedDisplayName() ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( GetTargetString( entity->GetLocalizedDisplayName(), i ) ); 
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
	}

	elementData.PushBack( TXT( "" ) );

	m_exportData.PushBack( elementData );
	
}

Bool CGameplayEntityLocalizationExporter::CanExportResource( CResource* resource )
{
	if ( resource == NULL )
	{
		return false;
	}
	CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( resource );
	if ( entityTemplate != NULL && entityTemplate->GetEntityObject() != NULL )
	{
		return entityTemplate->GetEntityObject()->IsA< CGameplayEntity >();
	}
	return false;
}

void CGameplayEntityLocalizationExporter::ExportBatchEntry( const String& entry )
{
	CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( entry );
	
	Bool canExport = false;
	if ( entityTemplate != NULL && entityTemplate->GetEntityObject() != NULL )
	{
		canExport = entityTemplate->GetEntityObject()->IsA< CGameplayEntity >();
	}

	if ( canExport == false )
	{
		return;
	}

	CGameplayEntity* entity = Cast< CGameplayEntity >( entityTemplate->GetEntityObject() );

	TDynArray< String > elementData;
	elementData.PushBack( GetResourceName( entityTemplate ) );
	elementData.PushBack( ToString( entity->GetLocalizedDisplayName().GetIndex() ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( entity->GetClass()->GetName().AsString() );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( GetSourceString( entity->GetLocalizedDisplayName() ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( GetTargetString( entity->GetLocalizedDisplayName(), i ) ); 
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
	}

	elementData.PushBack( TXT( "" ) );

	m_exportData.PushBack( elementData );
}

Bool CGameplayEntityLocalizationExporter::CanExportFile( CDiskFile* file ) const
{
	return file != NULL && file->GetFileName().EndsWith( TXT( ".w2ent" ) );
	//CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( file->GetDepotPath() );

	//Bool canExport = false;
	//if ( entityTemplate != NULL && entityTemplate->GetEntityObject() != NULL )
	//{
	//	canExport = entityTemplate->GetEntityObject()->IsA< CGameplayEntity >();
	//}
	//return canExport;
}

CGameplayEntityLocalizationExporter::CGameplayEntityLocalizationExporter()
	: AbstractLocalizationExporter()
{
	FillRootExportGroup( m_rootExportGroup );
}

Bool CGameplayEntityLocalizationExporter::IsBatchEntryValid( const String& entry ) const
{
	// nothing checked - entry is valid
	return true;
}

Bool CGameplayEntityLocalizationExporter::ValidateBatchEntry( const String& entry )
{
	// nothing checked, nothing to validate - entry is valid
	return true;
}

void CGameplayEntityLocalizationExporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{
	TDynArray< String > csvRow;
	csvRow.PushBack( entry );
	csvRow.PushBack( errorMsg );
	m_errorData.PushBack( csvRow );
}
