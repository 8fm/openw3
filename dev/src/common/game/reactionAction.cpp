/**
* Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "reactionAction.h"

#include "interestPointComponent.h"
#include "questsSystem.h"

IMPLEMENT_ENGINE_CLASS( IReactionAction );

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CReactionSendEvent );

void CReactionSendEvent::Perform( CNewNPC* npc, CInterestPointInstance* interestPointInstance, Int32 reactionIndex )
{
	npc->SignalGameplayEvent( m_eventName, interestPointInstance, CInterestPointInstance::GetStaticClass() );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CReactionQuestNotification );

void CReactionQuestNotification::Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex )
{
	GCommonGame->GetSystem< CQuestsSystem >()->OnReactionExecuted( npc, interestPoint );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CReactionAttitudeChange );

void CReactionAttitudeChange::Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex )
{
	CActor* targetActor = NULL;

	if( m_towardSource )
	{	
		// get the npc that generated this reaction ( if any )
		CNode* node = interestPoint->GetNode().Get();
		if ( node && node->IsA< CActor >() )
		{
			targetActor = Cast< CActor >( node );
		}		
	}
	else
	{
		targetActor = GCommonGame->GetPlayer();
	}

	if( targetActor )
	{
		npc->SetAttitude( targetActor, m_attitude );
		if( m_noticeActor )
		{
			npc->NoticeActor( targetActor );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CReactionScript );

void CReactionScript::Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex )
{
	CallFunction( this, CNAME( Perform ), THandle< CNewNPC >( npc ), THandle< CInterestPointInstance >( interestPoint ), reactionIndex );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CReactionLookAtAction );

void CReactionLookAtAction::Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex )
{
	if ( m_lookAtType != RLT_None )
	{
		const CNode* node = interestPoint->GetNode().Get();
		if ( node )
		{
			if ( node->IsA< CNewNPC>() )
			{
				const CNewNPC* target = Cast< const CNewNPC >( node );

				const CAnimatedComponent* ac = target->GetRootAnimatedComponent();
				Int32 bone = target->GetHeadBone();

				if ( ac && bone != -1 )
				{
					SLookAtReactionBoneInfo info;
					info.m_targetOwner = ac;
					info.m_boneIndex = bone;
					info.m_type = m_lookAtType;
					info.m_reactionPriority = 0;

					npc->EnableLookAt( info );
				}
				else
				{
					ASSERT( ac && bone != -1 );
				}
			}
			else
			{
				SLookAtReactionDynamicInfo info;
				info.m_target = node;
				info.m_type = m_lookAtType;
				info.m_reactionPriority = 0;

				npc->EnableLookAt( info );
			}
		}
		else
		{
			SLookAtReactionStaticInfo info;
			info.m_target = interestPoint->GetPosition();
			info.m_type = m_lookAtType;
			info.m_reactionPriority = 0;

			npc->EnableLookAt( info );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CReactionVoiceAction );

void CReactionVoiceAction::Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex )
{
	if ( !m_voiceset.Empty() )
	{
		// Cooldown test
		const EngineTime& currentTime = GGame->GetEngineTime();
		Float lastTime = npc->GetReactionLastVoicesetPlayedTime( reactionIndex );
		if( m_cooldown == 0.0f || lastTime == 0.0f || currentTime >= lastTime + m_cooldown )
		{
			npc->SetReactionLastVoicesetPlayedTime( reactionIndex, GGame->GetEngineTime() );
			GCommonGame->GetPlayer()->PlayVoicesetForNPC( npc, m_voiceset );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CReactionMultiAction );

void CReactionMultiAction::Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex )
{
	for ( TDynArray<IReactionAction*>::iterator it = m_actions.Begin(); it != m_actions.End(); ++it )
		(*it)->Perform( npc, interestPoint, reactionIndex );
}

///////////////////////////////////////////////////////////////////////////////
