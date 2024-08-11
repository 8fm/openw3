/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/game/storySceneSystem.h"
#include "../../common/game/storySceneAnimationList.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneAnimationParams.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneIncludes.h"

#include "dialogEditorAnimationPropertyEditor.h"
#include "dialogTimeline.h"
#include "dialogEditor.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "dialogEventGeneratorSetupDialog.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/animatedIterators.h"
#include "../../common/engine/skeletalAnimationContainer.h"

//////////////////////////////////////////////////////////////////////////

IAnimationSelectionByNameEditor::IAnimationSelectionByNameEditor( CPropertyItem* propertyItem )
	: ISelectionEditor( propertyItem )
{

}

void IAnimationSelectionByNameEditor::FillChoices()
{
	static_cast<wxItemContainer*>( m_ctrlChoice )->Clear();
	m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );

	const CAnimatedComponent* animatedComponent = RetrieveAnimationComponent();

	if( !animatedComponent )
	{
		ASSERT( false && "There is no animated component for this actor. Possible Causes: - no dialogset, no template in actor definition, no specified voice tag in actor template" );
		return;
	}

	for ( ComponentAnimsetIterator animsetIter( animatedComponent ); animsetIter; ++animsetIter )
	{
		const TDynArray< CSkeletalAnimationSetEntry* >& animationEntries = (*animsetIter)->GetAnimations();

		for ( TDynArray< CSkeletalAnimationSetEntry* >::const_iterator entryIter = animationEntries.Begin();
			entryIter != animationEntries.End(); ++entryIter )
		{
			Int32 insertedIndex = AppendChoiceIfAnimationIsValid( *entryIter );
			if ( insertedIndex != -1 )
			{
				m_ctrlChoice->SetClientObject( insertedIndex, new CAnimationEntryDataItem( *entryIter ) );
			}
		}
	}
}

Bool IAnimationSelectionByNameEditor::SaveValue()
{
	CAnimationEntryDataItem* selectedData = static_cast< CAnimationEntryDataItem* >( m_ctrlChoice->GetClientObject( m_ctrlChoice->GetSelection() ) );
	if ( selectedData != NULL && selectedData->m_animationEntry != NULL )
	{
		OnAnimationSelected( selectedData->m_animationEntry );
		return true;
	}
	else
	{
		OnAnimationSelected( NULL );
	}
	return false;
}

CAnimatedComponent* IAnimationSelectionByNameEditor::RetrieveAnimationComponent() const
{
	CStorySceneEvent *animationEvent = m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >();
	CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
	return sceneEditor->GetAnimatedComponentForActor( animationEvent->GetSubject() );
}

//////////////////////////////////////////////////////////////////////////

CEdDialogAnimationSelection::CEdDialogAnimationSelection( CPropertyItem* propertyItem )
	: IAnimationSelectionEditor( propertyItem )
{	
}

CAnimatedComponent* CEdDialogAnimationSelection::RetrieveAnimationComponent() const
{
	CName actor;
	if( CStorySceneEvent* animationEvent = m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >() )
	{
		actor = animationEvent->GetSubject();
	}
	else if( IDialogBodyAnimationFilterInterface* bodyFilter = dynamic_cast< IDialogBodyAnimationFilterInterface* >( m_propertyItem->GetRootObject( 0 ).AsObject() ) )
	{
		actor = bodyFilter->GetBodyFilterActor();
	}

	CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
	return sceneEditor->GetAnimatedComponentForActor( actor );
}

//////////////////////////////////////////////////////////////////////////

CEdDialogBodyAnimationFilter::CEdDialogBodyAnimationFilter( CPropertyItem* propertyItem, Uint32 categoryLevel )
	: ISelectionEditor( propertyItem )
	, m_categoryLevel( categoryLevel )
{

}

void CEdDialogBodyAnimationFilter::FillChoices()
{
	static_cast<wxItemContainer*>( m_ctrlChoice )->Clear();

	if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_BODY_ANIMATIONS )
	{
		m_ctrlChoice->AppendString( String::EMPTY.AsChar() );
	}
	else
	{
		m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );
	}

	const IDialogBodyAnimationFilterInterface* iter = nullptr;
	if ( !iter )
	{
		iter = dynamic_cast< IDialogBodyAnimationFilterInterface* >( m_propertyItem->GetRootObject( 0 ).As< CObject >() );
	}
	if ( !iter )
	{
		iter = dynamic_cast< IDialogBodyAnimationFilterInterface* >( m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >() );
	}
	SStorySceneActorAnimationGenData byValue;
	if ( !iter )
	{
		SStorySceneActorAnimationGenData* ptr = m_propertyItem->GetParentObject( 0 ).As< SStorySceneActorAnimationGenData >();
		if ( ptr )
		{
			byValue = *ptr;
			iter = &byValue;
		}				
	}
	if ( !iter )
	{
		iter = dynamic_cast< IDialogBodyAnimationFilterInterface* >( m_propertyItem->GetParentObject( 0 ).As< CStorySceneDialogsetSlot >() );
	}

	const CName status = iter->GetBodyFilterStatus();
	const CName emotionalState = iter->GetBodyFilterEmotionalState();
	const CName poseName = iter->GetBodyFilterPoseName();
	const CName typeName = iter->GetBodyFilterTypeName();

	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
	if ( !list.IsValid() )
	{
		list.ShowErrors( GFeedback );
	}

	if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_BODY_STATUS )
	{
		THashSet< CName > uniqueStatuses;
		for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
		{
			if ( ( it.GetEmoState() == emotionalState || emotionalState == CName::NONE ) && 
				( it.GetPose() == poseName || poseName == CName::NONE ) &&
				( it.GetTypeName() == typeName || typeName == CName::NONE ) )
			{
				if ( !uniqueStatuses.Exist( it.GetStatus() ) )
				{
					uniqueStatuses.Insert( it.GetStatus() );
					m_ctrlChoice->AppendString( it.GetStatus().AsString().AsChar() );
				}
			}
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_BODY_EMOTIONAL_STATE )
	{
		THashSet< CName > uniqueEmoStates;
		for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
		{
			if ( ( it.GetStatus() == status || status == CName::NONE ) && 
				( it.GetPose() == poseName || poseName == CName::NONE ) &&
				( it.GetTypeName() == typeName || typeName == CName::NONE ) )
			{
				if ( !uniqueEmoStates.Exist( it.GetEmoState() ) )
				{
					uniqueEmoStates.Insert( it.GetEmoState() );
					m_ctrlChoice->AppendString( it.GetEmoState().AsString().AsChar() );
				}
			}
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_BODY_POSE )
	{
		THashSet< CName > uniquePoses;
		for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
		{
			if ( ( it.GetStatus() == status || status == CName::NONE ) && 
				 ( it.GetEmoState() == emotionalState || emotionalState == CName::NONE ) &&
				 ( it.GetTypeName() == typeName || typeName == CName::NONE ) )
			{
				if ( !uniquePoses.Exist( it.GetPose() ) )
				{
					uniquePoses.Insert( it.GetPose() );
					m_ctrlChoice->AppendString( it.GetPose().AsString().AsChar() );
				}
			}
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_BODY_TYPE )
	{
		THashSet< CName > uniqueTypes;
		for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
		{
			if ( ( it.GetStatus() == status || status == CName::NONE ) && 
				( it.GetEmoState() == emotionalState || emotionalState == CName::NONE ) &&
				( it.GetPose() == poseName || poseName == CName::NONE ) )
			{
				if ( !uniqueTypes.Exist( it.GetTypeName() ) )
				{
					uniqueTypes.Insert( it.GetTypeName() );
					m_ctrlChoice->AppendString( it.GetTypeName().AsString().AsChar() );
				}
			}
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_BODY_ANIMATIONS )
	{
		CEdSceneEditor* mediator = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
		if ( !mediator )
		{
			return;
		}
		CAnimatedComponent* animatedComponent = mediator->OnDialogAnimationFilter_GetBodyComponentForActor( iter->GetBodyFilterActor() );
		if ( !animatedComponent || !animatedComponent->GetAnimationContainer() )
		{
			return;
		}

		const CSkeletalAnimationContainer* cont = animatedComponent->GetAnimationContainer();
		for ( CStorySceneAnimationList::BodyAnimationIterator it( list, status, emotionalState, poseName, typeName ); it; ++it )
		{
			// TODO it can be slow
			if ( cont->HasAnimation( (*it).m_animationName ) )
			{
				m_ctrlChoice->AppendString( (*it).m_friendlyName.AsChar() );
			}
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_BODY_TRANSITION )
	{
		CEdSceneEditor* mediator = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
		if ( !mediator )
		{
			return;
		}

		SStorySceneActorAnimationState currState;
		if ( mediator->OnDialogAnimationFilter_GetPreviousActorAnimationState( iter->GetBodyFilterActor(), currState ) )
		{
			SStorySceneActorAnimationState destState( currState );

			destState.m_status = status;
			destState.m_emotionalState = emotionalState;
			destState.m_poseType = poseName;

			TDynArray< CName > transitions;
			if ( GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindBodyTransitions( currState, destState, transitions ) )
			{
				for ( Uint32 i=0; i<transitions.Size(); ++i )
				{
					m_ctrlChoice->AppendString( transitions[ i ].AsChar() );
				}
			}
		}
	}
	else
	{
		ASSERT( 0 );
	}
}

void CEdDialogBodyAnimationFilter::OnChoiceChanged( wxCommandEvent &event )
{
	ISelectionEditor::OnChoiceChanged( event );
	RunLaterOnce( [ this ](){ m_propertyItem->GetPage()->RefreshValues(); } );
}

//////////////////////////////////////////////////////////////////////////

CEdDialogMimicsAnimationFilter::CEdDialogMimicsAnimationFilter( CPropertyItem* propertyItem, Uint32 categoryLevel )
	: ISelectionEditor( propertyItem )
	, m_categoryLevel( categoryLevel )
{

}

void CEdDialogMimicsAnimationFilter::FillChoices()
{
	static_cast<wxItemContainer*>( m_ctrlChoice )->Clear();

	/*if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_MIMICS_ANIMATIONS )
	{
		m_ctrlChoice->AppendString( String::EMPTY.AsChar() );
	}
	else
	{*/
		m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );
	/*}*/

	const IDialogMimicsAnimationFilterInterface* iter = dynamic_cast< IDialogMimicsAnimationFilterInterface* >( m_propertyItem->GetRootObject( 0 ).As< CObject >() );
	if ( !iter )
	{
		iter = dynamic_cast< IDialogMimicsAnimationFilterInterface* >( m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >() );
	}
	if ( !iter )
	{
		iter = dynamic_cast< IDialogMimicsAnimationFilterInterface* >( m_propertyItem->GetRootObject( 0 ).As< CStorySceneDialogsetSlot >() );
	}

	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
	if ( !list.IsValid() )
	{
		list.ShowErrors( GFeedback );
	}

	if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_MIMICS_ACTION_TYPE )
	{
		for ( CStorySceneAnimationList::ActionTypeMimicsIterator it( list ); it; ++it )
		{
			m_ctrlChoice->AppendString( (*it).AsString().AsChar() );
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_MIMICS_ANIMATIONS )
	{
		CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
		if ( !sceneEditor )
		{
			return;
		}
		CAnimatedComponent* animatedComponent = sceneEditor->OnDialogAnimationFilter_GetMimicsComponentForActor( iter->GetMimicsFilterActor() );
		if ( !animatedComponent || !animatedComponent->GetAnimationContainer() )
		{
			return;
		}
		const CSkeletalAnimationContainer* cont = animatedComponent->GetAnimationContainer();
		const CName actionFilter = iter->GetMimicsActionFilter();
		for ( CStorySceneAnimationList::MimicsAnimationIteratorByAction it( list, actionFilter ); it; ++it )
		{
			// TODO it can be slow
			if ( cont->HasAnimation( (*it).m_animationName ) )
			{
				m_ctrlChoice->AppendString( (*it).m_friendlyName.AsChar() );
			}
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_MIMICS_EMOTIONAL_STATE )
	{
		for ( CStorySceneAnimationList::EmotionalStateMimicsIterator it( list ); it; ++it )
		{
			m_ctrlChoice->AppendString( (*it).m_emoState.AsChar() );
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_MIMICS_LAYER_EYES )
	{
		for ( CStorySceneAnimationList::LayerMimicsAnimationIterator it( list, CStorySceneAnimationList::LAYER_EYES ); it; ++it )
		{
			m_ctrlChoice->AppendString( (*it).m_friendlyName.AsChar() );
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_MIMICS_LAYER_POSE )
	{
		for ( CStorySceneAnimationList::LayerMimicsAnimationIterator it( list, CStorySceneAnimationList::LAYER_POSE ); it; ++it )
		{
			m_ctrlChoice->AppendString( (*it).m_friendlyName.AsChar() );
		}
	}
	else if ( m_categoryLevel == CStorySceneAnimationList::LEVEL_MIMICS_LAYER_ANIMATION )
	{
		for ( CStorySceneAnimationList::LayerMimicsAnimationIterator it( list, CStorySceneAnimationList::LAYER_ANIMATION ); it; ++it )
		{
			m_ctrlChoice->AppendString( (*it).m_friendlyName.AsChar() );
		}
	}
	else
	{
		ASSERT( 0 );
	}
}

void CEdDialogMimicsAnimationFilter::OnChoiceChanged( wxCommandEvent &event )
{
	ISelectionEditor::OnChoiceChanged( event );
	RunLaterOnce( [ this ](){ m_propertyItem->GetPage()->RefreshValues(); } );
}

//////////////////////////////////////////////////////////////////////////

CEdDialogMimicAnimationSelection::CEdDialogMimicAnimationSelection( CPropertyItem* propertyItem )
	: IAnimationSelectionEditor( propertyItem )
{

}

CAnimatedComponent* CEdDialogMimicAnimationSelection::RetrieveAnimationComponent() const
{
	CStorySceneEvent* animationEvent = m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >();
	CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
	return sceneEditor->OnMimicsAnimationSelector_GetHeadComponentForActor( animationEvent->GetSubject() );
}

//////////////////////////////////////////////////////////////////////////

CEdDialogCameraAnimationSelection::CEdDialogCameraAnimationSelection( CPropertyItem* propertyItem )
	: IAnimationSelectionEditor( propertyItem )
{

}

CAnimatedComponent* CEdDialogCameraAnimationSelection::RetrieveAnimationComponent() const
{
	CStorySceneEvent* animationEvent = m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >();
	CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
	return sceneEditor->OnCameraAnimationSelector_GetCameraComponent();
}

//////////////////////////////////////////////////////////////////////////

void CEdDialogExitAnimationSelection::FillChoices()
{
	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
	if ( CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem ) )
	{
		if ( CStorySceneEventExitActor* evt = m_propertyItem->GetParentObject( 0 ).As< CStorySceneEventExitActor >() )
		{			
			if( CAnimatedComponent*	ac = sceneEditor->OnDialogAnimationFilter_GetBodyComponentForActor( evt->GetActor() ) )
			{
				if( CSkeletalAnimationContainer* cont = ac->GetAnimationContainer() )
				{
					for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
					{			
						if( it.GetTypeName() == CStorySceneAnimationList::EXIT_KEYWORD  && cont->HasAnimation( (*it).m_animationName ) )
						{
							m_ctrlChoice->AppendString( (*it).m_animationName.AsChar() );
						}		
					}
				}				
			}			
		}
	}
}

CEdDialogExitAnimationSelection::CEdDialogExitAnimationSelection( CPropertyItem* propertyItem ) : ISelectionEditor( propertyItem )
{
}

void CEdDialogEnterAnimationSelection::FillChoices()
{
	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
	if ( CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem ) )
	{
		if ( CStorySceneEventEnterActor* evt = m_propertyItem->GetParentObject( 0 ).As< CStorySceneEventEnterActor >() )
		{			
			if( CAnimatedComponent*	ac = sceneEditor->OnDialogAnimationFilter_GetBodyComponentForActor( evt->GetActor() ) )
			{
				if( CSkeletalAnimationContainer* cont = ac->GetAnimationContainer() )
				{
					for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
					{			
						if( it.GetTypeName() == CStorySceneAnimationList::ENTER_KEYWORD && cont->HasAnimation( (*it).m_animationName ) )
						{
							m_ctrlChoice->AppendString( (*it).m_animationName.AsChar() );
						}		
					}
				}				
			}			
		}
	}
}

CEdDialogEnterAnimationSelection::CEdDialogEnterAnimationSelection( CPropertyItem* propertyItem ) : ISelectionEditor( propertyItem )
{
}
