/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorSetupFormation.h"

#include "formation.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorSetupFormationDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetupFormationInstance
////////////////////////////////////////////////////////////////////////

CBehTreeNodeDecoratorSetupFormationInstance::CBehTreeNodeDecoratorSetupFormationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_runtimeData( owner )
	, m_formation( def.m_formation.GetVal( context ) )
{
	if ( m_formation )
	{
		m_owner->AddDependentObject( m_formation );
	}
}
void CBehTreeNodeDecoratorSetupFormationInstance::OnDestruction()
{
	if ( m_formation )
	{
		m_owner->RemoveDependentObject( m_formation );
	}

	Super::OnDestruction();
}

Bool CBehTreeNodeDecoratorSetupFormationInstance::Activate()
{
	CActor* leader = Cast< CActor >( m_owner->GetActionTarget().Get() );
	if ( !leader || !m_formation )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_runtimeData->Setup( m_formation, leader, m_owner->GetActor() );

	if ( !Super::Activate() )
	{
		m_runtimeData->Clear();
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}
void CBehTreeNodeDecoratorSetupFormationInstance::Deactivate()
{
	Super::Deactivate();

	m_runtimeData->Clear();
}

void CBehTreeNodeDecoratorSetupFormationInstance::Update()
{
	m_runtimeData->Update( m_owner );

	Super::Update();
}