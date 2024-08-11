/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "enableDeniedAreaAction.h"

#include "../engine/deniedAreaComponent.h"

IMPLEMENT_ENGINE_CLASS( CEnabledDeniedAreaAction )

void CEnabledDeniedAreaAction::PerformOnComponent( CComponent* component )
{
	static_cast< CDeniedAreaComponent* >( component )->SetEnabled( m_enable );
}
CClass* CEnabledDeniedAreaAction::SupportedComponentClass()
{
	return CDeniedAreaComponent::GetStaticClass();
}