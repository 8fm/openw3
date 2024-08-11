#include "build.h"
#include "environmentManager.h"
#include "skyTransformComponent.h"
#include "tickManager.h"
#include "world.h"
#include "entity.h"
#include "layer.h"

IMPLEMENT_RTTI_ENUM( ESkyTransformType );
IMPLEMENT_ENGINE_CLASS( CSkyTransformComponent );

CSkyTransformComponent::CSkyTransformComponent()
	: m_lastCameraPos( 0, 0, 0 )
	, m_transformType( STT_SunPosition )
	, m_cameraDistance( 975.f )
	, m_uniformScaleDistance( 20.f )
{
}

void CSkyTransformComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetTickManager()->AddToGroup( this, TICK_Main );	
}

void CSkyTransformComponent::OnDetached( CWorld* world )
{
	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );	
	TBaseClass::OnDetached( world );
}

void CSkyTransformComponent::OnTick( Float timeDelta )
{	
	// Recaculate transform every frame	
	ScheduleUpdateTransformNode();
}

Bool CSkyTransformComponent::CalcWorldGlobalTransformMatrix( Matrix& matrix ) const
{
	if ( IsAttached() )
	{
		// We need to be attached
		CWorld* attachedWorld = GetEntity()->GetLayer()->GetWorld();
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

		// Build matrix
		matrix.SetIdentity();
		matrix.SetTranslation( dir * m_cameraDistance );
		matrix.SetScale33( m_cameraDistance / Max(0.05f, m_uniformScaleDistance) );
		return true;
	}

	// Not calculated
	return false;
}
