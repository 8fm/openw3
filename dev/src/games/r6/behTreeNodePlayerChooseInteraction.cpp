/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behTreeNodePlayerChooseInteraction.h"
#include "../../common/game/behTreeInstance.h"
#include "r6AimingComponent.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodePlayerChooseInteractionDefinition
////////////////////////////////////////////////////////////////////////


CBehTreeNodePlayerChooseInteractionDefinition::CBehTreeNodePlayerChooseInteractionDefinition()
{
}



IBehTreeNodeDecoratorInstance* CBehTreeNodePlayerChooseInteractionDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}











////////////////////////////////////////////////////////////////////////
// CBehTreeNodePlayerChooseInteractionInstance
////////////////////////////////////////////////////////////////////////








CBehTreeNodePlayerChooseInteractionInstance::CBehTreeNodePlayerChooseInteractionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
{

}










Bool CBehTreeNodePlayerChooseInteractionInstance::IsAvailable()
{
	if( IsActive() )
	{
		return Super::IsAvailable();
	}

	R6_ASSERT( GetOwner() );
	CEntity* entity = GetOwner()->GetEntity();
	R6_ASSERT( entity );

	CR6AimingComponent* aimingComponent = Cast< CR6AimingComponent >( entity->FindComponent< CR6AimingComponent >() );
	if( !aimingComponent )
	{
		DebugNotifyAvailableFail();
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( GetOwner()->GetActor() ), TXT( "Behaviour Tree" ), TXT( "PlayerChooseInteraction requires the player entity to contain CR6AimingComponent." ) );
		return false;
	}

	CEntity* aimedEntity = aimingComponent->GetAimedEntity();
	if( !aimedEntity )
	{
		DebugNotifyAvailableFail();
		return false;
	}



	//
	// @todo MS: Fill CBehTreeNodePlayerChooseInteractionInstance::IsAvailable()
	// 
	// Interaction choice is not only the entity to be interacted but also its specific interaction component.
	// That's why we have to make a choice based on HUD player input decision.
	//
	// This might be done either here or in CR6AimingComponent or somewhere else. Not yet decided. (MS)
	//
	// Notice that previous choice implementation was done in function SetFocusMenu( _visible : bool ) of r6Player.ws.
	//

	m_owner->SetActionTarget( aimedEntity );


	return Super::IsAvailable();
}
