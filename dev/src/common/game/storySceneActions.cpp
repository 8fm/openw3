/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneActions.h"
#include "storySceneDirector.h"
#include "storyScenePlayer.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneAction );

Bool CStorySceneAction::Perform( CStorySceneDirector* director, Float timePassed ) const
{
	/*CStorySceneDialogsetSlot*		dialogSetSlot = Cast<CStorySceneDialogsetSlot>(GetParent());
	CStorySceneDialogsetInstance*	dialogSet	  = FindParent<CStorySceneDialogsetInstance>();
	CName actorName;
	m_entity = m_actor = NULL;
	if ( dialogSetSlot && dialogSet )
	{
		actorName	= dialogSetSlot->GetActorName();
		m_entity	= director->GetDirectorParent()->GetSceneActorEntity(actorName, false);
		m_actor		= Cast<CActor>(m_entity);
	}
	if ( m_entity == NULL ) // || m_aState == A_Finished || m_aState == A_Failed )
	{
		return true;
	}
	if( IsCompleted() )
	{
		return true;
	}
	if( DoPerform() == false )
	{	
		return DoFail();
	}
	//if ( timePassed - m_timeStarted > m_maxTime )
	//{
	//	return DoFail();
	//}
	*/
	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActionTeleport );

Bool CStorySceneActionTeleport::Perform( CStorySceneDirector* director, Float timePassed ) const
{
	/*CStorySceneDialogsetSlot*		dialogSetSlot = Cast<CStorySceneDialogsetSlot>(GetParent());
	CStorySceneDialogsetInstance*	dialogSet	  = FindParent<CStorySceneDialogsetInstance>();
	CName actorName;
	if( dialogSetSlot && dialogSet )
	{
		director->GetSlotPlacement(dialogSetSlot, dialogSet, m_targetSpot);
	}
	return CStorySceneAction::Perform(director, timePassed );*/
	return false;
}

Bool CStorySceneActionTeleport::DoPerform() const
{
	m_entity->Teleport( m_targetSpot.GetPosition(), m_targetSpot.GetRotation() );
	if ( m_actor )
	{
		m_actor->ActionCancelAll();
	}
	return true;
}

Bool CStorySceneActionTeleport::IsCompleted() const
{
	return m_entity->GetWorldPositionRef().DistanceSquaredTo( m_targetSpot.GetPosition() ) < m_allowedDistance * m_allowedDistance;
}

Bool CStorySceneActionTeleport::DoFail() const
{
	m_entity->Teleport( m_targetSpot.GetPosition(), m_targetSpot.GetRotation() );
	if ( m_actor )
	{
		m_actor->ActionCancelAll();
	}
	return CStorySceneAction::DoFail();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActionMoveTo );

Bool CStorySceneActionMoveTo::DoPerform() const
{
	return m_actor && m_actor->ActionMoveTo( m_targetSpot.GetPosition(), m_targetSpot.GetRotation().Yaw ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActionSlide );

Bool CStorySceneActionSlide::DoPerform() const
{
	return m_actor && m_actor->ActionSlideTo( m_targetSpot.GetPosition(), m_targetSpot.GetRotation().Yaw, m_slideTime, SR_Nearest );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActionStopWork );

Bool CStorySceneActionStopWork::DoPerform() const
{
	return m_actor && m_actor->ActionExitWorking() ;
}

Bool CStorySceneActionStopWork::IsCompleted()	const
{
	if ( m_actor )
	{
		return ( m_actor->GetActionType() &  ActorAction_Working ) == false;
	}
	return true;
}

Bool CStorySceneActionStopWork::DoFail()	const
{
	return m_actor && CStorySceneAction::DoFail() && m_actor->ActionExitWorking(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActionStartWork );

Bool CStorySceneActionStartWork::DoPerform() const
{
	return  m_actor && m_jobTree && m_actor->ActionStartWorking( m_jobTree.Get(), m_category, false );
}

Bool CStorySceneActionStartWork::IsCompleted() const
{
	if ( m_actor )
	{
		return ( m_actor->GetActionType() &  ActorAction_Working) == false;
	}
	return true;
}

Bool CStorySceneActionStartWork::DoFail() const
{
	return m_actor && m_jobTree &&  CStorySceneAction::DoFail() && m_actor->ActionStartWorking( m_jobTree.Get(), m_category, true );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActionRotateToPlayer );

Bool CStorySceneActionRotateToPlayer::DoPerform() const
{
	Vector player = GCommonGame->GetPlayer()->GetWorldPosition();
	return m_actor && m_actor->ActionRotateTo( player ) ;
}

Bool CStorySceneActionRotateToPlayer::IsCompleted() const
{
	Vector toPlayer = GCommonGame->GetPlayer()->GetWorldPosition() - m_entity->GetPosition();
	EulerAngles angles = m_entity->GetWorldRotation();
	return angles.Yaw - toPlayer.ToEulerAngles().Yaw < m_acceptableAngleDif ;
}

Bool CStorySceneActionRotateToPlayer::DoFail() const
{
	Vector toPlayer = GCommonGame->GetPlayer()->GetWorldPosition() - m_entity->GetPosition();
	EulerAngles angles = m_entity->GetWorldRotation();
	angles.Yaw = toPlayer.ToEulerAngles().Yaw;
	m_entity->SetRotation( angles );
	CActor* actor = Cast< CActor >( m_entity );
	if ( m_actor )
	{
		actor->ActionCancelAll();
	}
	return CStorySceneAction::DoFail();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActionEquipItem );

Bool CStorySceneActionEquipItem::DoPerform() const
{
	if( m_actor )
	{
		SActorRequiredItems info(m_leftHandItem, m_rightHandItem);
		m_actor->IssueRequiredItems( info );
		return true;
	}
	return false;
}

Bool CStorySceneActionEquipItem::IsCompleted() const
{
	if(m_actor)
	{
		CInventoryComponent *	invComponent = m_actor->GetInventoryComponent();  
		SItemUniqueId			id;
		const SInventoryItem *	item;
		if( invComponent )
		{
			id = invComponent->GetItemIdHeldInSlot( CNAME( l_weapon ) ) ;
			item =	invComponent->GetItem(id);
			if( m_leftHandItem != CNAME(Any)  && !( item && item->GetName() == m_leftHandItem )  && m_leftHandItem != CName::NONE ) 
			{
				return false;
			}
			id = invComponent->GetItemIdHeldInSlot( CNAME( r_weapon ) ) ;
			item = invComponent->GetItem(id);
			if( m_rightHandItem != CNAME(Any) && !( item && item->GetName() == m_rightHandItem ) && m_rightHandItem != CName::NONE ) 
			{
				return false;
			}
		}
	}
	return true;
}

Bool CStorySceneActionEquipItem::DoFail() const
{
	if( m_actor )
	{
		SActorRequiredItems info(m_leftHandItem, m_rightHandItem);
		m_actor->IssueRequiredItems( info, true );
	}
	return CStorySceneAction::DoFail();
}

