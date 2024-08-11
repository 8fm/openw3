/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionIsInInterior.h"
#include "r4Player.h"
#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/game/communitySystem.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionAmIInInteriorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsPlayerInInteriorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionAmIOrAPInInteriorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeIgnoreInteriorsDuringPathfindingDefinition )
	

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionAmIInInteriorInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionAmIInInteriorInstance::ConditionCheck()
{
	CNewNPC* npc = m_owner->GetNPC();
	if ( !npc )
	{
		return false;
	}
	return npc->IsInInterior();
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsPlayerInInteriorInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionIsPlayerInInteriorInstance::ConditionCheck()
{
	CR4Player* player = Cast< CR4Player >( GGame->GetPlayerEntity() );
	if ( !player )
	{
		return false;
	}

	return player->IsInInterior();
}

CBehTreeNodeConditionAmIOrAPInInteriorInstance::CBehTreeNodeConditionAmIOrAPInInteriorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_workData( owner )
{}

Bool CBehTreeNodeConditionAmIOrAPInInteriorInstance::ConditionCheck()
{
	CNewNPC* npc = m_owner->GetNPC();
	if ( !npc )
	{
		return false;
	}

	if ( npc->IsInInterior() )
	{
		return true;
	}

	SActionPointId apId = m_workData->GetSelectedAP();
	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	Vector position;
	if ( actionPointManager->GetActionExecutionPosition( apId, &position, nullptr ) )
	{
		return GR4Game->IsPositionInInterior( position );
	}

	return false;
}

Bool CBehTreeNodeIgnoreInteriorsDuringPathfindingInstance::Activate()
{
	CPathAgent* pathAgent = GetPathAgent();
	if ( pathAgent )
	{
		pathAgent->AddForbiddenPathfindFlag( PathLib::NF_INTERIOR );
	}

	if ( !Super::Activate() )
	{
		return false;
	}

	return true;
}

void CBehTreeNodeIgnoreInteriorsDuringPathfindingInstance::Deactivate()
{
	CPathAgent* pathAgent = GetPathAgent();
	if ( pathAgent )
	{
		pathAgent->RemoveForbiddenPathfindFlag( PathLib::NF_INTERIOR );
	}

	Super::Deactivate();
}

CPathAgent* CBehTreeNodeIgnoreInteriorsDuringPathfindingInstance::GetPathAgent() const
{
	CActor* actor = GetOwner()->GetActor();
	if ( actor )
	{
		CMovingAgentComponent* movingAgent = actor->GetMovingAgentComponent();
		if ( movingAgent )
		{
			return movingAgent->GetPathAgent();
		}
	}

	return nullptr;
}
