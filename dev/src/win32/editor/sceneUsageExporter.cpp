/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "sceneUsageExporter.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneComponent.h"
#include "../../common/game/questPhase.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questPhaseBlock.h"
#include "../../common/game/questContextDialogBlock.h"
#include "../../common/game/questInteractionDialogBlock.h"
#include "../../common/game/questSceneBlock.h"

#include "../../common/core/depot.h"

CSceneUsageExporter::CSceneUsageExporter()
{
	FillRootExportGroup( m_rootExportGroup );
}

void CSceneUsageExporter::BeginBatchExport()
{
	m_usedScenes.Clear();
	m_exploredGraphs.Clear();
	m_directories.Clear();
}

void CSceneUsageExporter::EndBatchExport()
{
	CDirectory* gameDirectory = GDepot->FindPath( TXT( "game\\" ) );
	
	TDynArray< String > usedScenes;
	TDynArray< String > unusedScenes;
	CollectScenesPaths( gameDirectory, usedScenes, unusedScenes );

	m_exportData.Clear();

	ExportSingleColumnLine( TXT( "Used scenes" ) );
	for( Uint32 i = 0; i < usedScenes.Size(); ++i )
	{
		ExportPath( usedScenes[ i ] );
	}

	ExportSingleColumnLine( String::EMPTY );

	ExportSingleColumnLine( TXT( "Unused scenes" ) );
	for( Uint32 j = 0; j < unusedScenes.Size(); ++j )
	{
		ExportPath( unusedScenes[ j ] );
	}
	
	String exportString = ConvertToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFileWithUTF8( m_savePath, exportString );
	}

	DumpErrorsFile();
}

void CSceneUsageExporter::ExportBatchEntry( const String& entry )
{
	CDirectory* directory = GDepot->FindPath( entry.AsChar() );
	ScanDirectoryForScene( directory, 1 );

	m_directories.PushBack( entry );
}

Bool CSceneUsageExporter::CanExportFile( CDiskFile* file ) const
{
	return file != NULL && file->GetDepotPath().EndsWith( GetResourceExtension() );
}

String CSceneUsageExporter::GetResourceExtension() const
{
	return TXT( "w2quest" );
}

void CSceneUsageExporter::CollectScenesPaths( CDirectory* directory, TDynArray<String>& usedScenes, TDynArray< String >& unusedScenes )
{
	if ( directory == NULL )
	{
		return;
	}

	const TFiles files = directory->GetFiles();
	for ( TFiles::const_iterator fileIter = files.Begin(); fileIter != files.End(); ++fileIter )
	{
		String filePath = (*fileIter)->GetDepotPath();
		if ( filePath.EndsWith( TXT( "w2scene" ) ) == false )
		{
			continue;
		}

		Bool isInRelevantDirectory = false;
		for ( Uint32 i = 0; i < m_directories.Size(); ++i )
		{
			if ( filePath.BeginsWith( m_directories[ i ] ) == true )
			{
				isInRelevantDirectory = true;
			}
		}

		if ( isInRelevantDirectory == false )
		{
			continue;
		}

		if ( m_usedScenes.Find( filePath ) == m_usedScenes.End() )
		{
			unusedScenes.PushBackUnique( filePath );
			LOG_EDITOR( TXT( "UNUSED SCENE: %s" ), filePath.AsChar() );
		}
		else
		{
			usedScenes.PushBackUnique( filePath );
			//LOG_EDITOR( TXT( "USED SCENE: %s" ), filePath.AsChar() );
		}
	}

	for ( CDirectory* subDirectory : directory->GetDirectories() )
	{
		CollectScenesPaths( subDirectory, usedScenes, unusedScenes );
	}
}

void CSceneUsageExporter::ExportSingleColumnLine( const String& text )
{
	TDynArray< String > lineData;
	lineData.PushBack( text );
	m_exportData.PushBack( lineData );
}

void CSceneUsageExporter::ExportPath( const String& path )
{
	TDynArray< String > lineData;
	lineData.PushBack( path.StringAfter( TXT( "\\" ), true ) ); 
	lineData.PushBack( path.StringBefore( TXT( "\\" ), true ) ); 
	m_exportData.PushBack( lineData );
}

void CSceneUsageExporter::FillRootExportGroup( BatchExportGroup& exportGroup )
{
	CDirectory* directory = GDepot->FindPath( TXT( "game\\" ) );
	if ( directory == NULL )
	{
		return;
	}

	exportGroup.m_groupName = TXT( "Root" );

	BatchExportGroup subGroup;
	
	subGroup.m_groupName = directory->GetName();

	for ( CDirectory * pDir : directory->GetDirectories() )
	{
		subGroup.m_groupEntries.PushBack( pDir->GetDepotPath() );
	}

	exportGroup.m_subGroups.PushBack( subGroup );
}

void CSceneUsageExporter::ScanDirectoryForScene( CDirectory* directory, Uint32 level /*= 0 */ )
{
	const TFiles files = directory->GetFiles();
	for ( TFiles::const_iterator fileIter = files.Begin(); fileIter != files.End(); ++fileIter )
	{
		CDiskFile* file = *fileIter;
		String filePath = file->GetDepotPath();

		CResource* resource = GDepot->LoadResource( filePath );

		if ( filePath.EndsWith( TXT( "w2quest" ) ) && level == 1 )
		{
			ScanQuestForScene( Cast< CQuestPhase >( resource ) );
		}
		else if ( filePath.EndsWith( TXT( "w2ent" ) ) )
		{
			ScanEntityForScenes( Cast< CEntityTemplate >( resource ) );
		}
		else if ( filePath.EndsWith( TXT( "w2comm" ) ) )
		{
			ScanCommunityForScene( Cast< CCommunity >( resource ) );
		}
	}

	for ( CDirectory * pDir : directory->GetDirectories() )
	{
		ScanDirectoryForScene( pDir, level + 1 );
	}
}

void CSceneUsageExporter::ScanEntityForScenes( CEntityTemplate* entityTemplate )
{
	if ( entityTemplate == NULL )
	{
		return;
	}

	CEntity* entity = entityTemplate->GetEntityObject();
	if ( entity == NULL )
	{
		return;
	}

	TDynArray< CStorySceneComponent* > sceneComponents;
	CollectEntityComponents( entity, sceneComponents );

	for ( Uint32 i = 0; i < sceneComponents.Size(); ++i )
	{
		CStorySceneComponent* sceneComponent = sceneComponents[ i ];

		TSoftHandle< CStoryScene > sceneHandle = sceneComponent->GetStoryScene();
		CStoryScene* scene = sceneHandle.Get();
		if ( scene != NULL )
		{
			m_usedScenes.Insert( scene->GetDepotPath() );
		}

		sceneHandle.Release();
	}
}

void CSceneUsageExporter::ScanQuestForScene( CQuestPhase* quest )
{
	if ( quest == NULL )
	{
		return;
	}

	CQuestGraph* questGraph = quest->GetGraph();

	TDynArray< CQuestGraph* > graphsStack;
	graphsStack.PushBack( questGraph );

	// analyze the graph
	while ( !graphsStack.Empty() )
	{
		CQuestGraph* currGraph = graphsStack.PopBack();
		if ( currGraph == NULL )
		{
			continue;
		}
		if ( m_exploredGraphs.Find( currGraph ) != m_exploredGraphs.End() )
		{
			continue;
		}
		else
		{
			m_exploredGraphs.Insert( currGraph );
		}

		TDynArray< CGraphBlock* >& blocks = currGraph->GraphGetBlocks();
		Uint32 count = blocks.Size();

		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( blocks[ i ]->IsA< CQuestContextDialogBlock >() )
			{
				CQuestContextDialogBlock* contextBlock = Cast< CQuestContextDialogBlock >( blocks[ i ] );
				if ( contextBlock->GetScene() != NULL )
				{
					m_usedScenes.Insert( contextBlock->GetScene()->GetDepotPath() );
				}
				
				if ( contextBlock->GetTargetScene() != NULL )
				{
					m_usedScenes.Insert( contextBlock->GetTargetScene()->GetDepotPath() );
				}
				
			}
			else if ( blocks[ i ]->IsA< CQuestInteractionDialogBlock >() )
			{
				CQuestInteractionDialogBlock* interactionBlock = Cast< CQuestInteractionDialogBlock >( blocks[ i ] );

				if ( interactionBlock->GetScene() != NULL )
				{
					m_usedScenes.Insert( interactionBlock->GetScene()->GetDepotPath() );
				}
			}
			else if ( blocks[ i ]->IsA< CQuestSceneBlock >() )
			{
				CQuestSceneBlock* sceneBlock = Cast< CQuestSceneBlock >( blocks[ i ] );
				if ( sceneBlock->GetScene() )
				{
					m_usedScenes.Insert( sceneBlock->GetScene()->GetDepotPath() );
				}
			}
			else if ( blocks[ i ]->IsA< CQuestScopeBlock >() )
			{
				CQuestScopeBlock* scope = Cast< CQuestScopeBlock >( blocks[ i ] );
				graphsStack.PushBack( scope->GetGraph() );
			}
		}
	}
}

void CSceneUsageExporter::ScanCommunityForScene( CCommunity* community )
{
	if ( community == NULL )
	{
		return;
	}

	const TDynArray< CSSceneTableEntry >& sceneEntries = community->GetScenesTable();
	for ( Uint32 i = 0; i < sceneEntries.Size(); ++i )
	{
		const TDynArray< CSSceneTimetableEntry > & sceneTimetableEntries = sceneEntries[ i ].m_timetable;
		for ( Uint32 j = 0; j < sceneTimetableEntries.Size(); ++j )
		{
			const TDynArray< CSSceneTimetableScenesEntry >& sceneTimetableScenes = sceneTimetableEntries[ j ].m_scenes;
			for ( Uint32 k = 0; k < sceneTimetableScenes.Size(); ++k )
			{
				CStoryScene* scene = sceneTimetableScenes[ k ].m_storyScene.Get();

				if ( scene != NULL )
				{
					m_usedScenes.Insert( scene->GetDepotPath() );
				}
			}
		}
	}
}

Bool CSceneUsageExporter::IsBatchEntryValid( const String& entry ) const
{
	// nothing checked - entry is valid
	return true;
}

Bool CSceneUsageExporter::ValidateBatchEntry( const String& entry )
{
	// nothing checked, nothing to validate - entry is valid
	return true;
}

void CSceneUsageExporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{
	TDynArray< String > csvRow;
	csvRow.PushBack( entry );
	csvRow.PushBack( errorMsg );
	m_errorData.PushBack( csvRow );
}
