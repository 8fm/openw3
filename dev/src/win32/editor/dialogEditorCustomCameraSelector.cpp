/*
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "dialogEditorCustomCameraSelector.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneSection.h"

void CEdCustomCameraSelector::FillChoices()
{	
	CStorySceneEventCustomCameraInstance* cameraEvent = m_propertyItem->GetParentObject( 0 ).As< CStorySceneEventCustomCameraInstance >();

	if ( cameraEvent == NULL )
	{
		return;
	}

	CStorySceneSection* section = cameraEvent->GetSceneElement()->GetSection();
	if( section == NULL )
	{
		return;
	}

	CStoryScene* scene = section->GetScene();
	if( scene == NULL )
	{
		return;
	}

	for( TDynArray< StorySceneCameraDefinition >::const_iterator animIter = scene->GetCameraDefinitions().Begin();
		animIter != scene->GetCameraDefinitions().End(); ++animIter )
	{
		const StorySceneCameraDefinition& camera = *animIter;
		m_ctrlChoice->AppendString( camera.m_cameraName.AsString().AsChar() );
	}
}
