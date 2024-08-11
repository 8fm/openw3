/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeAtomicFlight.h"

#include "../../common/game/behTreeInstance.h"
#include "../../common/engine/renderFrame.h"

///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeAtomicFlightInstance
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeAtomicFlightInstance::IBehTreeNodeAtomicFlightInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_flightData( owner )
{}
Bool IBehTreeNodeAtomicFlightInstance::Activate()
{
	CActor* actor = m_owner->GetActor();
	m_flightData->Activate( actor );

	return Super::Activate();		// true
}
void IBehTreeNodeAtomicFlightInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	m_flightData->Deactivate( actor );

	Super::Deactivate();
}
void IBehTreeNodeAtomicFlightInstance::Update()
{
	// check if we can fly to desired position straight
	CBehTreeFlightData* flightData = m_flightData.Get();
	if ( !flightData->UpdateFlight( this ) )
	{
		Complete( BTTO_FAILED );
	}
}
void IBehTreeNodeAtomicFlightInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	CBehTreeFlightData* flightData = m_flightData.Get();

	const auto& detailedPath = flightData->GetCurrentDetailedPath();
	if ( !detailedPath.Empty() )
	{
		for ( Int32 i = detailedPath.Size()-2; i >= 0; --i )
		{
			frame->AddDebugLine( detailedPath[ i+1 ], detailedPath[ i ], Color::WHITE, true );
		}
	}

	const auto& currentPath = flightData->GetCurrentPath();
	if ( !currentPath.Empty() )
	{
		frame->AddDebugLine( m_owner->GetActor()->GetWorldPositionRef(), currentPath.Back(), Color::RED, true );
		for ( Int32 i = currentPath.Size()-2; i >= 0; --i )
		{
			frame->AddDebugLine( currentPath[ i+1 ], currentPath[ i ], Color::GREEN, true );
		}
	}


}

CAreaComponent*	IBehTreeNodeAtomicFlightInstance::GetAreaEncompassingMovement()
{
	return nullptr;
}