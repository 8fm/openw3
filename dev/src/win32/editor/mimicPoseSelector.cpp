/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "mimicPoseSelector.h"
#include "dialogEditor.h"

#include "../../common/game/storySceneEvent.h"
#include "../../common/engine/mimicComponent.h"

CEdSceneMimicPoseSelector::CEdSceneMimicPoseSelector( CPropertyItem* item, Bool filterSelection )
	: ISelectionEditor( item )
	, m_filterSelection( filterSelection )
{
	CStorySceneEvent* sceneEvent = m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >();
	CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
	m_headComponent = Cast< CMimicComponent >( sceneEditor->OnMimicPoseSelector_GetHeadComponentForActor( sceneEvent->GetSubject() ) );
}

void CEdSceneMimicPoseSelector::FillChoices()
{
	m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );
	
	if( m_headComponent.Get() && m_headComponent.Get()->GetMimicFace() )
	{
		CExtendedMimics mimics = m_headComponent.Get()->GetExtendedMimics();

		if ( m_filterSelection == false )
		{
			for( Uint32 i = 0, numPoses = mimics.GetNumTrackPoses(); i < numPoses; ++i )
			{
				String item = mimics.GetTrackPoseName( i ).AsString();
				//if ( item.BeginsWith( TXT("pose__") ) )
				{
					m_ctrlChoice->AppendString( item.AsChar() );
				}
			}
		}
		else
		{
			for( Uint32 i = 0, numPoses = mimics.GetNumFilterPoses(); i < numPoses; ++i )
			{
				String item = mimics.GetFilterPoseName( i ).AsString();
				m_ctrlChoice->AppendString( item.AsChar() );
			}
		}
	}
}
