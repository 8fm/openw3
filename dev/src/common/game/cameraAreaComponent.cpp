/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cameraAreaComponent.h"

#if 0
IMPLEMENT_ENGINE_CLASS( CCameraAreaComponent );

CCameraAreaComponent::CCameraAreaComponent()
	: m_isPlayerInside( false )
{
}

void CCameraAreaComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );
}

void CCameraAreaComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );
}

void CCameraAreaComponent::OnTickPrePhysics( Float timeDelta )
{
	// Pass to base class
	TBaseClass::OnTickPrePhysics( timeDelta );

#if 0
	// Make sure player is inside when this is called
	if ( m_isPlayerInside )
	{
		// Send the camera event
		if ( m_tickEvent )
		{
			CCamera* camera = GGame->GetCamera();
			if ( camera )
			{
				camera->RaiseBehaviorEvent( m_tickEvent );
			}		
		}
	}
#endif
}

void CCameraAreaComponent::EnteredArea( CComponent* component )
{
	// Player entered the trigger
	if ( component && component->GetEntity() == GGame->GetPlayerEntity() )
	{
		ASSERT( !m_isPlayerInside );

		// Send the event to camera
		if ( m_enterEvent )
		{
			CCamera* camera = GGame->GetCamera();
			if ( camera )
			{
				camera->RaiseBehaviorEvent( m_enterEvent );
			}	
		}

		// Start auto timer
		if ( m_tickEvent )
		{
			GetLayer()->GetWorld()->GetTickManager()->AddToGroup( this, TICK_PrePhysics );
		}

		// Set flag
		m_isPlayerInside = true;
	}
}

void CCameraAreaComponent::ExitedArea( CComponent* component )
{
	// Player exited the trigger
	if ( component && component->GetEntity() == GGame->GetPlayerEntity() )
	{
		ASSERT( m_isPlayerInside );

		// Send exit event to camera
		if ( m_exitEvent )
		{
			CCamera* camera = GGame->GetCamera();
			if ( camera )
			{
				camera->RaiseBehaviorEvent( m_exitEvent );
			}	
		}

		// End auto timer
		if ( m_tickEvent )
		{
			GetLayer()->GetWorld()->GetTickManager()->RemoveFromGroup( this, TICK_PrePhysics );
		}

		// Reset flag
		m_isPlayerInside = false;
	}
}
#endif