#include "build.h"

#include "sceneDefinitionNode.h"
#include "reactionSceneActor.h"
#include "behTreeInstance.h"
#include "behTreeNodeConditionReactionEvent.h"
#include "behTreeReactionData.h"
#include "../engine/renderFrame.h"

IBehTreeNodeDecoratorInstance* CBehTreeNodeReactionSceneDefinitionDecorator::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{			
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeReactionSceneDefinitionDecoratorInstance::CBehTreeNodeReactionSceneDefinitionDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionReactionEventInstance( def, owner, context, parent )
	, m_roles( def.m_roles )
	, m_workBeingPerformed( owner, CNAME( WorkBeingPerformedCounter ) )
	, m_softReactionLocker( owner, CNAME( SoftReactionsLocker ) )
	, m_inInWorkBranch( def.m_inInWorkBranch.GetVal( context ) )	
{	
	m_setActionTarget = false;
}

Bool CBehTreeNodeReactionSceneDefinitionDecoratorInstance::CanBeAssignedToScene( CBehTreeReactionEventData* reactionData )
{
	CActor* ownerActor = m_owner->GetActor();
	CReactionSceneActorComponent* actorCmp = ownerActor->FindComponent< CReactionSceneActorComponent >();
	if( !actorCmp->CanPlayInScene() )
	{
		return false;
	}	

	if( ownerActor->IsAtWork( ) && ownerActor->GetCurrentJTType() == 1 ) //EJTT_Praying
	{
		return false;
	}

	if( reactionData->GetEventName() != CNAME( GreetingAction ) )
	{
		// if not greeting at least one actor must stand in place
		CActor* invoker = Cast< CActor >( reactionData->GetInvoker() );
		return invoker && !( ownerActor->IsMoving() && invoker->IsMoving() );
	}

	return true;
}

Bool CBehTreeNodeReactionSceneDefinitionDecoratorInstance::OnListenedEvent( CBehTreeEvent& e )
{
	CActor* npc = m_owner->GetActor();

	if ( e.m_gameplayEventData.m_customDataType == CBehTreeReactionEventData::GetStaticClass() 
		&& !m_parent->IsMoreImportantNodeActive( this ) 
		&& ( !npc->IsAtWork() || ( m_inInWorkBranch && npc->CanUseChatInCurrentAP() ) ) )
	{
		CBehTreeReactionEventData* reactionData = static_cast< CBehTreeReactionEventData* >( e.m_gameplayEventData.m_customData );
		if( reactionData && reactionData->GetReactionScene() )
		{
			CReactionScene* reactionScene = reactionData->GetReactionScene();
			reactionScene->InitializeRoles( m_roles );
			
			if( reactionScene->GetScenePhase() == ERSEP_Assigning )
			{
				if( CanBeAssignedToScene( reactionData ) )
				{
					CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
					reactionScene->AssigntToFirstFreeRole( actorCmp );
				}								
			}
			else
			{
				return Super::OnListenedEvent( e );
			}					
		}
	}

	return false;
}

Bool CBehTreeNodeReactionSceneDefinitionDecoratorInstance::Activate()
{
	bool ret = Super::Activate();

	if( ret )
	{
		m_softReactionLocker->ModifyCounter( 1 );
	}

	return ret;
}

void CBehTreeNodeReactionSceneDefinitionDecoratorInstance::Deactivate()
{	
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	if( actorCmp )
	{
		actorCmp->FinishRole( true );
	}	

	if( m_softReactionLocker->GetCounter() > 0 )
	{
		m_softReactionLocker->ModifyCounter( -1 );
	}	
	Super::Deactivate();
}

void CBehTreeNodeReactionSceneDefinitionDecoratorInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();

	if( actorCmp && actorCmp->GetRole() )
	{
		Vector textPos = m_owner->GetActor()->GetWorldPosition();
		textPos.Y += 2;
		frame->AddDebugText( textPos, actorCmp->GetRole().AsString(), 0, 0, true, Color::WHITE );

		CReactionScene* scene = actorCmp->GetReactionScene();
		if( scene && scene->GetInvoker() )
		{
			CEntity* invoker = scene->GetInvoker()->GetEntity();
			if( invoker != m_owner->GetActor() )
			{
				Vector targetPos = invoker->GetWorldPosition();
				targetPos.Y += 2;
				frame->AddDebugLineWithArrow( textPos, targetPos, 0.5f, 0.1f, 0.1f, Color::RED, true );
			}			
		}

	}
}

Bool CBehTreeNodeReactionSceneDefinitionDecoratorInstance::ConditionCheck()
{
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();	

	if( !actorCmp )
		return false;

	if( actorCmp->IfSceneInterupted() )
	{		
		return false;
	}
	
	CBehTreeReactionEventData* reactionData = m_reactionData.Get();
	// Reaction data tests
	return reactionData && !reactionData->HasExpired();	
}