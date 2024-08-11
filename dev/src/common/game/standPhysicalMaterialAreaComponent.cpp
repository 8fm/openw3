/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "standPhysicalMaterialAreaComponent.h"

IMPLEMENT_ENGINE_CLASS( CStandPhysicalMaterialAreaComponent );

CStandPhysicalMaterialAreaComponent::CStandPhysicalMaterialAreaComponent ()
 : m_physicalMaterialName( CName::NONE )
{}

void CStandPhysicalMaterialAreaComponent::EnteredArea( CComponent* component )
{
	CMovingAgentComponent* movingAgentComponent = Cast< CMovingAgentComponent >( component );
	if( !movingAgentComponent ) return;

	const SPhysicalMaterial* physicalMaterial = GPhysicEngine->GetMaterial( m_physicalMaterialName );

	movingAgentComponent->ForceStandPhysicalMaterial( physicalMaterial );
}

void CStandPhysicalMaterialAreaComponent::ExitedArea( CComponent* component )
{
	CMovingAgentComponent* movingAgentComponent = Cast< CMovingAgentComponent >( component );
	if( !movingAgentComponent ) return;

	const SPhysicalMaterial* physicalMaterial = GPhysicEngine->GetMaterial( m_physicalMaterialName );

	movingAgentComponent->ReleaseStandPhysicalMaterial(physicalMaterial);
}

Color CStandPhysicalMaterialAreaComponent::CalcLineColor() const
{
	return Color::LIGHT_GREEN;
}

void CStandPhysicalMaterialAreaComponent::OnAttached(CWorld* world)
{
	m_includedChannels |= TC_Horse;

	TBaseClass::OnAttached( world );
}

