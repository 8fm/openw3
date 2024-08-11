/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "customSceneExporters.h"

#include "../../../common/game/storySceneFlowCondition.h"
#include "../../../common/game/storySceneFlowSwitch.h"
#include "../../../common/game/questCondition.h"
#include "../../../common/game/storySceneChoice.h"
#include "../../../common/game/storySceneChoiceLine.h"

#include "../../../common/core/depot.h"

CSceneFactsExporter::CSceneFactsExporter()
{
	FillRootExportGroup( m_rootExportGroup );
}

void CSceneFactsExporter::ExportFlowCondition( CStoryScene* storyScene, const CStorySceneFlowCondition* flowCondition )
{
	if ( flowCondition == NULL || flowCondition->GetCondition() == NULL )
	{
		return;
	}

	const IQuestCondition* condition = flowCondition->GetCondition();
	if ( condition == NULL )
	{
		return;
	}

	TDynArray< String > conditionExportData;
	conditionExportData.PushBack( storyScene->GetFile()->GetDepotPath() );
	conditionExportData.PushBack( flowCondition->GetFriendlyName() );
	conditionExportData.PushBack( flowCondition->GetComment() );
	conditionExportData.PushBack( condition->GetFriendlyName() );

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	conditionExportData.PushBack( condition->GetDescription() );
#endif

	m_exportData.PushBack( conditionExportData );

}

void CSceneFactsExporter::ExportFlowSwitch( CStoryScene* storyScene, const CStorySceneFlowSwitch* flowSwitch )
{
	if ( flowSwitch == NULL || ( flowSwitch->GetDefaultLink() == NULL && flowSwitch->GetCases().Empty() ) )
	{
		return;
	}

	TDynArray< String > conditionExportData;
	conditionExportData.PushBack( storyScene->GetFile()->GetDepotPath() );
	conditionExportData.PushBack( flowSwitch->GetFriendlyName() );
	conditionExportData.PushBack( flowSwitch->GetComment() );
	const TDynArray< CStorySceneFlowSwitchCase* >& cases = flowSwitch->GetCases();
	for( TDynArray< CStorySceneFlowSwitchCase* >::const_iterator it = cases.Begin(), end = cases.End() ;
		it != end ;
		++it )
	{
		if( (*it) != NULL && (*it)->m_whenCondition != NULL )
			conditionExportData.PushBack( (*it)->m_whenCondition->GetFriendlyName() );
#ifndef NO_EDITOR_PROPERTY_SUPPORT
		conditionExportData.PushBack( (*it)->m_whenCondition->GetDescription() );
#endif
	}

	m_exportData.PushBack( conditionExportData );
}

void CSceneFactsExporter::DoExportStorySceneChoice( const CStorySceneChoice* sectionChoice, CStoryScene* storyScene )
{
	for ( Uint32 i = 0; i < sectionChoice->GetNumberOfChoiceLines(); ++i )
	{
		const CStorySceneChoiceLine* choiceLine = sectionChoice->GetChoiceLine( i );

		if ( choiceLine->HasCondition() == true )
		{
			const IQuestCondition* condition = choiceLine->GetCondition();

			TDynArray< String > conditionExportData;
			conditionExportData.PushBack( storyScene->GetFile()->GetDepotPath() );
			conditionExportData.PushBack( sectionChoice->GetSection()->GetName() );
			conditionExportData.PushBack( GetSourceString( *( choiceLine->GetLocalizedChoiceLine() ) ) );
			conditionExportData.PushBack( condition->GetFriendlyName() );
#ifndef NO_EDITOR_PROPERTY_SUPPORT
			conditionExportData.PushBack( condition->GetDescription() );
#endif

			m_exportData.PushBack( conditionExportData );
		}
		if ( choiceLine->HasMemo() )
		{
			const TDynArray< ISceneChoiceMemo* >& memos = choiceLine->GetMemos();
			for ( Uint32 j = 0; j < memos.Size(); ++j )
			{
				ISceneChoiceMemo* memo = memos[ j ];

				if ( memo == NULL )
				{
					continue;
				}

				TDynArray< String > memoExportData;
				memoExportData.PushBack( storyScene->GetFile()->GetDepotPath() );
				memoExportData.PushBack( sectionChoice->GetSection()->GetName() );
				memoExportData.PushBack( GetSourceString( *( choiceLine->GetLocalizedChoiceLine() ) ) );
				memoExportData.PushBack( memo->GetFriendlyName() );

#ifndef NO_EDITOR_PROPERTY_SUPPORT
				memoExportData.PushBack( memo->GetDescription() );
#endif

				m_exportData.PushBack( memoExportData );

			}	
		}

	}
}

void CSceneFactsExporter::BeginBatchExport()
{
	TDynArray< String >	headerExportData;
	headerExportData.PushBack( TXT( "Resource path" ) );
	headerExportData.PushBack( TXT( "Section name" ) );
	headerExportData.PushBack( TXT( "Choice line" ) );
	headerExportData.PushBack( TXT( "Type" ) );
	headerExportData.PushBack( TXT( "Description" ) );

	m_exportData.PushBack( headerExportData );

	PrepareErrorFileHeaders();
}

void CSceneFactsExporter::EndBatchExport()
{
	String exportString = ConvertToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFile( m_savePath, exportString );
	}
}

void CSceneFactsExporter::ExportResource( CResource* resource )
{
	CStoryScene* storyScene = Cast< CStoryScene >( resource );
	if ( storyScene != NULL )
	{
		ExportScene( storyScene );
	}
}

Bool CSceneFactsExporter::CanExportResource( CResource* resource )
{
	if ( resource == NULL )
	{
		return false;
	}
	return resource->IsA< CStoryScene >();
}

void CSceneFactsExporter::ExportBatchEntry( const String& entry )
{
	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );
	if ( storyScene != NULL )
	{
		ExportScene( storyScene );
	}

}

Bool CSceneFactsExporter::IsBatchEntryValid( const String& entry ) const
{
	// nothing checked - entry is valid
	return true;
}

Bool CSceneFactsExporter::ValidateBatchEntry( const String& entry )
{
	// nothing checked, nothing to validate - entry is valid
	return true;
}

void CSceneFactsExporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{
	TDynArray< String > csvRow;
	csvRow.PushBack( entry );
	csvRow.PushBack( errorMsg );
	m_errorData.PushBack( csvRow );
}
