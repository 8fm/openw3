/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "interactionTooltipComponent.h"

IMPLEMENT_ENGINE_CLASS( CInteractionTooltipComponent );

CInteractionTooltipComponent::CInteractionTooltipComponent()
{
	m_rangeMax = 20.0f;
}

void CInteractionTooltipComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
}

void CInteractionTooltipComponent::OnDetached( CWorld *world )
{
	TBaseClass::OnDetached( world );
}

void CInteractionTooltipComponent::OnActivate( CEntity* activator )
{
	if ( !IsActive() )
	{
#if 0 // GFx 3
		CHudInstance* hud = GWitcherGame->GetHudInstance();
		if ( hud )
		{
			hud->InvokeMethod( "ShowEntity", (Float) hud->EntityToFlashHandle( GetEntity() ) );
		}
#endif // #if 0
	}

	TBaseClass::OnActivate( activator );

}

void CInteractionTooltipComponent::OnDeactivate( CEntity* activator )
{
	TBaseClass::OnDeactivate( activator );

	if ( !IsActive() )
	{
#if 0 // GFx 3
		CHudInstance* hud = GWitcherGame->GetHudInstance();
		if ( hud )
		{
			hud->InvokeMethod( "HideEntity", (Float) hud->EntityToFlashHandle( GetEntity() ) );
		}
#endif // #if 0
	}
}

Bool CInteractionTooltipComponent::VisibilityTest( const Vector& activatorPos ) const
{
	ASSERT( GGame->GetActiveWorld() );
	return TBaseClass::LineOfSightTest( GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition() );
}
