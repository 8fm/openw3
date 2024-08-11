/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "game.h"
#include "world.h"
#include "entity.h"
#include "renderCommands.h"
#include "triggerAreaEnvironmentVisibilityComponent.h"

IMPLEMENT_ENGINE_CLASS( CTriggerAreaEnvironmentVisibilityComponent );

CTriggerAreaEnvironmentVisibilityComponent::CTriggerAreaEnvironmentVisibilityComponent() 
{
}

void CTriggerAreaEnvironmentVisibilityComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Cache world for terrain / foliage proxy access
	m_world = world;
}

void CTriggerAreaEnvironmentVisibilityComponent::OnDetached(CWorld* world)
{
	ShowEnvironmentElements();

	TBaseClass::OnDetached( world );
}

void CTriggerAreaEnvironmentVisibilityComponent::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );
}

void CTriggerAreaEnvironmentVisibilityComponent::OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator )
{
	TBaseClass::OnActivatorEntered( object, activator );

	HideEnvironmentElements();
}

void CTriggerAreaEnvironmentVisibilityComponent::OnActivatorExited(const class ITriggerObject* object, const class ITriggerActivator* activator)
{
	TBaseClass::OnActivatorExited( object, activator );

	ShowEnvironmentElements();
}

void CTriggerAreaEnvironmentVisibilityComponent::HideEnvironmentElements()
{
	if( m_world )
	{
		( new CRenderCommand_SetupEnvironmentElementsVisibility( m_world->GetRenderSceneEx(), !m_hideTerrain, !m_hideFoliage, !m_hideWater ) )->Commit();
	}
}

void CTriggerAreaEnvironmentVisibilityComponent::ShowEnvironmentElements()
{
	if( m_world )
	{
		( new CRenderCommand_SetupEnvironmentElementsVisibility( m_world->GetRenderSceneEx(), true, true, true ) )->Commit();
	}
}