#include "build.h"
#include "game.h"
#include "environmentManager.h"
#include "skyTransformEntity.h"
#include "skyTransformComponent.h"
#include "world.h"
#include "tickManager.h"
#include "layer.h"


IMPLEMENT_ENGINE_CLASS( CSkyTransformEntity );

CSkyTransformEntity::CSkyTransformEntity()
: m_transformType( STT_SunPosition )
, m_alignToPlayer( false )
, m_onlyYaw( false )
{
}

void CSkyTransformEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetTickManager()->AddEntity( this );	
}

void CSkyTransformEntity::OnDetached( CWorld* world )
{
	world->GetTickManager()->RemoveEntity( this );	
	TBaseClass::OnDetached( world );
}

void CSkyTransformEntity::OnTick( Float timeDelta )
{	
	PC_SCOPE_PIX( SkyTransformEntityTick );

	if ( IsAttached() )
	{
		// We need to be attached
		CWorld* attachedWorld = GetLayer()->GetWorld();
		ASSERT( attachedWorld );

		// Get shit
		CEnvironmentManager *envManager	= attachedWorld->GetEnvironmentManager();

		// Calculate direction
		Vector dir = Vector::EZ;
		switch ( m_transformType )
		{
		case STT_SunPosition:
			{
				dir = envManager->GetSunDirection();
				break;
			}

		case STT_MoonPosition:
			{
				dir = envManager->GetMoonDirection();
				break;
			}

		case STT_GlobalLightPosition:
			{
				dir = envManager->GetGlobalLightDirection();
				break;
			}


		default:
			{
				ASSERT ( !"invalid_transform_type" );
			}
		}
		EulerAngles angles = dir.ToEulerAngles();

		if ( m_onlyYaw )
		{
			angles.Pitch = 0.0f;
			angles.Roll = 0.0f;
		}
		else
		{
			// some kind of magic...
			angles.Pitch = 270.0f-angles.Pitch;
			angles.Normalize();
		}

		m_transform.SetRotation( angles );

		// Align to player ? 
		if ( m_alignToPlayer && GGame && GGame->IsActive() && GGame->GetPlayerEntity() )
		{
			m_transform.SetPosition( GGame->GetPlayerEntity()->GetWorldPosition() );
		}

		// Recaculate transform every frame	
		ScheduleUpdateTransformNode();
	}
}
