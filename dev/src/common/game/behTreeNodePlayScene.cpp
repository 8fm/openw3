#include "build.h"
#include "behTreeNodePlayScene.h"

#include "behTreeInstance.h"
#include "movementAdjustor.h"
#include "sceneStopReasons.h"

IMPLEMENT_ENGINE_CLASS( SPlaySceneRequestData );
const CName& SPlaySceneRequestData::EVENT_ID = CNAME( AI_PlaySceneRequest );

IBehTreeNodeInstance* CBehTreeNodePlaySceneDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodePlaySceneInstance::CBehTreeNodePlaySceneInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )
	, m_isGameplayScene( false )
{
	m_priority = -1;
	
	SBehTreeEvenListeningData e;
	e.m_eventName = SPlaySceneRequestData::EVENT_ID;
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}
void CBehTreeNodePlaySceneInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = SPlaySceneRequestData::EVENT_ID;
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}
Bool CBehTreeNodePlaySceneInstance::Activate()
{
	// TODO: notice cutscene system
	CActor* actor = m_owner->GetActor();
	if ( CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
	{
		if ( CMovementAdjustor* adjustor = mac->GetMovementAdjustor() )
		{
			adjustor->CancelAll();
		}
	}

	return Super::Activate();
}
void CBehTreeNodePlaySceneInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	actor->StopAllScenes( SCR_SCENE_ENDED );
	m_isGameplayScene = false;
	m_priority = -1;
	Super::Deactivate();
}

Bool CBehTreeNodePlaySceneInstance::IsAvailable()
{
	if( m_priority > 0 )
	{
		return true;
	}
	else
	{
		DebugNotifyAvailableFail();
		return false;
	}
}
Int32 CBehTreeNodePlaySceneInstance::Evaluate()
{
	return m_priority;
}

Bool CBehTreeNodePlaySceneInstance::OnListenedEvent( CBehTreeEvent& e )
{
	SPlaySceneRequestData* data = e.m_gameplayEventData.Get< SPlaySceneRequestData >();
	if ( data )
	{
		if ( data->m_enablePlayScene )
		{
			m_isGameplayScene = data->m_isGameplayScene;
			m_priority = data->m_scenePriority;
		}
		else
		{
			m_isGameplayScene = false;
			m_priority = -1;
		}
		
		
		return true;
	}
	return false;
}

Bool CBehTreeNodePlaySceneInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_ForceLastWork ) )
	{	
		m_isGameplayScene = false;
		m_priority = -1;

		return true;
	}
	return false;
}