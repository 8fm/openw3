#include "build.h"

#include "actionPointActivationSwitcher.h"
#include "actionPointComponent.h"
#include "communitySystem.h"

IMPLEMENT_ENGINE_CLASS( CActionPointActivationSwitcher );

void CActionPointActivationSwitcher::PerformOnComponent( CComponent* component )
{
	if ( CActionPointComponent* ap = Cast< CActionPointComponent >( component ) )
	{
		ap->SetActive( m_activate );
	}
}

CClass* CActionPointActivationSwitcher::SupportedComponentClass()
{
	return CActionPointComponent::GetStaticClass();
}