#include "build.h"

#include "behTreeNodeWorkDecorator.h"
#include "actionPointComponent.h"
#include "actionPointManager.h"
#include "communitySystem.h"

IBehTreeNodeDecoratorInstance*	CBehTreeNodeWorkDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeWorkDecoratorInstance::CBehTreeNodeWorkDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )	
	, m_workBeingPerformed( owner, CNAME( WorkBeingPerformedCounter ) )
	, m_workData( owner )
	, m_softReactionLocker( owner, CNAME( SoftReactionsLocker ) )
{		
}

Bool CBehTreeNodeWorkDecoratorInstance::Activate()
{
	if( Super::Activate() )
	{
		m_workBeingPerformed->ModifyCounter( 1 );	
		m_currentApId = GetCurrentAPId();

		CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		if( CActionPointComponent* executedAP = actionPointManager->GetAP( m_currentApId ) )
		{			
			executedAP->WorkStarted();			
			if( executedAP->GetDisableSoftRactions() )
			{
				m_softReactionLocker->ModifyCounter( 1 );
			}
		}	

		return true;
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeNodeWorkDecoratorInstance::Deactivate()
{
	Super::Deactivate();
	m_workBeingPerformed->ModifyCounter( -1 );
	TActionPointID currentApId = GetCurrentAPId();

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem )
	{
		CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
		CActionPointComponent* executedAP = actionPointManager->GetAP( currentApId );
		if( !executedAP )
		{
			executedAP = actionPointManager->GetAP( m_currentApId );
		}
		
		if( executedAP )
		{
			executedAP->WorkEnded();

			if( executedAP->GetDisableSoftRactions() )
			{
				if( m_softReactionLocker->GetCounter() > 0 )
				{
					m_softReactionLocker->ModifyCounter( -1 );
				}				
			}
		}
	}
	
}


IBehTreeNodeInstance* CBehTreeNodeCustomWorkDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
CBehTreeNodeCustomWorkInstance::CBehTreeNodeCustomWorkInstance(  const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent  )
	: CBehTreeDynamicNodeInstance( def, owner, context, parent )
	,  m_workData( owner )
{
}

CAIPerformCustomWorkTree* CBehTreeNodeCustomWorkInstance::GetCustomBehTree()
{
	CBehTreeWorkData* workData = m_workData.Get();
	TActionPointID currentApId = workData->GetSelectedAP();

	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if( CActionPointComponent* executedAP = actionPointManager->GetAP( currentApId ) )
	{
		CAIPerformCustomWorkTree* behTree = executedAP->GetCustomWorkTree();
		return behTree;
	}
	return nullptr;
}

Bool CBehTreeNodeCustomWorkInstance::IsAvailable()
{	
	CAIPerformCustomWorkTree* behTree = GetCustomBehTree();
	if( behTree == nullptr )
	{
		DebugNotifyAvailableFail();
	}

	return behTree != nullptr;		
}

Bool CBehTreeNodeCustomWorkInstance::Activate()
{	
	CAIPerformCustomWorkTree* behTree = GetCustomBehTree();
	
	if( behTree )
	{
		SpawnChildNode( behTree, SBehTreeDynamicNodeEventData::Parameters(), this, m_owner );
		if( Super::Activate() )
		{
			return true;
		}
		else
		{
			DespawnChildNode();
			DebugNotifyActivationFail();
			return false;
		}
	}

	DebugNotifyActivationFail();
	return false;
}
