/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneAnimationReporter.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"

#include "../../common/game/storySceneAnimationList.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/core/gatheredResource.h"

CEdSceneAnimationReporter::CEdSceneAnimationReporter()
{
	FillRootExportGroup( m_rootExportGroup );
}

void CEdSceneAnimationReporter::DoExportSceneSectionHeader( const CStorySceneSection* section, CStoryScene* storyScene )
{
	const TDynArray< CStorySceneEvent* > & events = section->GetEventsFromAllVariants();
	Int32 numel = events.Size();
	Int32 j;
	for( j=0;j<numel;++j )
	{
		if( CStorySceneEventAnimClip* animEvt = Cast< CStorySceneEventAnimClip >( events[j] ) )
		{
			CName animname = animEvt->GetAnimationName();
			if ( animname ) animations.Insert( animname );
		}
		if( CStorySceneEventChangePose* animEvt = Cast< CStorySceneEventChangePose >( events[j] ) )
		{
			CName animname = animEvt->GetForceBodyIdleAnimation();
			if ( animname ) animations.Insert( animname );
		}
	}
}

void CEdSceneAnimationReporter::BeginBatchExport()
{
	m_exportData.Clear();
	animations.Clear();

	const C2dArray& arrBody		= SSceneAnimationsResourcesManager::GetInstance().GetSceneAnimationsBody2dArray();
	const C2dArray& arrMimics	= SSceneAnimationsResourcesManager::GetInstance().GetSceneAnimationsMimics2dArray();

	if( arrBody.Empty() == false )
	{
		const Uint32 size = (Uint32)arrBody.GetNumberOfRows();
		for ( Uint32 i=0; i<size; ++i )
		{
			String animname = arrBody.GetValueRef( 4, i ).TrimCopy();
			if( animname.Size()>0 ) { animations.Insert( CName(animname) ); }
			String animname2 = arrBody.GetValueRef( 7, i ).TrimCopy();
			if( animname2.Size()>0 ) { animations.Insert( CName(animname2)); }
		}
	}
	if( arrMimics.Empty() == false )
	{
		const Uint32 size = (Uint32)arrMimics.GetNumberOfRows();
		for ( Uint32 i=0; i<size; ++i )
		{
			String animname = arrMimics.GetValueRef( 4, i ).TrimCopy();
			if( animname.Size()>0 ) { animations.Insert( CName(animname) ); }
			String animname2 = arrMimics.GetValueRef( 7, i ).TrimCopy();
			if( animname2.Size()>0 ) { animations.Insert( CName(animname2)); }
		}
	}

}

void CEdSceneAnimationReporter::EndBatchExport()
{
	Int32 numanims = animations.Size();
	for( THashSet<CName>::iterator it = animations.Begin(); it!=animations.End(); ++it )
	{
		TDynArray< String > destinationData;
		destinationData.PushBack( (*it).AsString() );
		m_exportData.PushBack( destinationData );
	}
	if ( m_exportData.Size() == 0 )
	{
		return;
	}
	String exportString = ConvertToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFileWithUTF8( m_savePath, exportString );
	}
}

void CEdSceneAnimationReporter::ExportScene( CStoryScene* storyScene )
{
	if ( storyScene == NULL )
	{
		return;
	}
	if ( IsBatchedExport() == false )
	{
		DoBeginExport();
	}

	TDynArray< CStorySceneSection* > sections;
	storyScene->CollectControlParts( sections );
	Int32 j;
	const Int32 numSceneElements = sections.Size();
	for ( j = 0; j < numSceneElements; ++j )
	{
		const CStorySceneSection* section = Cast< const CStorySceneSection >( sections[ j ] );
		if ( section != NULL )
		{
			ExportSceneSection( storyScene, section );
		}
	}
	if ( IsBatchedExport() == false )
	{
		DoEndExport();
	}
}

void CEdSceneAnimationReporter::ExportResource( CResource* resource )
{
	CStoryScene* storyScene = Cast< CStoryScene >( resource );
	if ( storyScene != NULL )
	{
		ExportScene( storyScene );
	}
}

Bool CEdSceneAnimationReporter::CanExportResource( CResource* resource )
{
	if ( resource == NULL )
	{
		return false;
	}
	return resource->IsA< CStoryScene >();
}

void CEdSceneAnimationReporter::ExportBatchEntry( const String& entry )
{
	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );
	if ( storyScene != NULL )
	{
		ExportScene( storyScene );
	}
}

Bool CEdSceneAnimationReporter::IsBatchEntryValid( const String& entry ) const
{
	Bool sceneValid = false;
	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );
	if ( storyScene )
	{
		sceneValid = true;
	}
	return sceneValid;
}

Bool CEdSceneAnimationReporter::ValidateBatchEntry( const String& entry )
{
	Bool sceneValid = false;

	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );
	if ( storyScene )
	{
		sceneValid = true;
	}

	return sceneValid;
}

void CEdSceneAnimationReporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{

}
