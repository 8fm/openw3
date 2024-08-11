#include "build.h"
#include "behTreeDecoratorRiderWaitHorseScriptedAction.h"

#include "../../common/game/aiActionParameters.h"

////////////////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderWaitHorseScriptedActionDefinition
////////////////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeRiderWaitHorseScriptedActionDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new CBehTreeNodeRiderWaitHorseScriptedActionInstance( *this, owner, context, parent );
}
///////////////////////////////////////////
CBehTreeNodeRiderWaitHorseScriptedActionInstance::CBehTreeNodeRiderWaitHorseScriptedActionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_initSucceded( false )
	, m_horseDecoratorTree( nullptr )
	, m_riderData( owner, CNAME( RiderData )  )
	, m_scriptedActionId( -1 )
	, m_horseActionFinished( false )
	, m_horseArrivedAtPath( false )
{
}

Bool CBehTreeNodeRiderWaitHorseScriptedActionInstance::Activate()
{
	if ( m_riderData == false)
	{
		DebugNotifyActivationFail();
		return false;
	}

	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CActor *const horse						= sharedParams->m_horse.Get();
	if ( horse == nullptr  )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CAIQuestScriptedActionsTree * horseDecoratorTree = m_horseDecoratorTree.Get();
	
	if ( horseDecoratorTree == nullptr )
	{
		horseDecoratorTree		= CAIQuestScriptedActionsTree::GetStaticClass()->CreateObject< CAIQuestScriptedActionsTree >();
		m_horseDecoratorTree	= horseDecoratorTree;

		horseDecoratorTree->InitializeTree();

		IAIActionTree *const aiActionTree = GetAITree();
		m_initSucceded = false;
		if ( aiActionTree )
		{
			horseDecoratorTree->InitializeData( aiActionTree );
			horseDecoratorTree->SetListenerParam( this );
			horseDecoratorTree->InitializeAIParameters();
			m_initSucceded = true;
		}
		
	}

	if ( m_initSucceded == false )
	{
		DebugNotifyActivationFail();
		return false;
	}
	
	if ( horse->ForceAIBehavior( horseDecoratorTree, BTAP_Emergency, &m_scriptedActionId ) == false )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( m_horseArrivedAtPath )
	{
		horse->SignalGameplayEvent( CNAME( ForceArriveAtPath ) );
	}

	m_horseActionFinished = false; // in case the node is used more than once
	return Super::Activate();
}
void CBehTreeNodeRiderWaitHorseScriptedActionInstance::Deactivate() 
{
	if ( m_riderData )
	{
		CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
		if ( sharedParams )
		{
			CActor *const horse							= sharedParams->m_horse.Get();
			if ( horse )
			{
				horse->CancelAIBehavior( m_scriptedActionId );
			}
		}
	}
	Super::Deactivate();
}

Bool CBehTreeNodeRiderWaitHorseScriptedActionInstance::IsSavingState() const
{
	return m_horseArrivedAtPath;
}

void CBehTreeNodeRiderWaitHorseScriptedActionInstance::SaveState( IGameSaver* writer )
{
	writer->WriteValue< Bool >( CNAME( ArrivedAtPath ), m_horseArrivedAtPath );
}

Bool CBehTreeNodeRiderWaitHorseScriptedActionInstance::LoadState( IGameLoader* reader )
{
	reader->ReadValue< Bool >( CNAME( ArrivedAtPath ), m_horseArrivedAtPath );
	return true;
}

void CBehTreeNodeRiderWaitHorseScriptedActionInstance::OnBehaviorEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( ArrivedAtPath ) )
	{
		m_horseArrivedAtPath = true;
	}
}

IAIActionTree * CBehTreeNodeRiderWaitHorseScriptedActionInstance::GetAITree()const
{
	if ( m_riderData == false )
	{
		return nullptr;
	}
	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		return nullptr;
	}
	IAIActionTree * horseScriptedActionTree = m_riderData->m_horseScriptedActionTree.Get();
	return horseScriptedActionTree;
}
void CBehTreeNodeRiderWaitHorseScriptedActionInstance::OnBehaviorCompletion( Bool success )
{
	m_horseActionFinished = true;
}
void CBehTreeNodeRiderWaitHorseScriptedActionInstance::Update()
{
	Super::Update(); // at the start because otherwise we will be updating an inactive child
	if ( m_horseActionFinished )
	{
		Complete( BTTO_SUCCESS );
	}

}