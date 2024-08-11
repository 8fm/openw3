/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../core/dataError.h"
#include "../core/depot.h"
#include "../core/gatheredResource.h"
#include "../engine/entityTemplate.h"
#include "../engine/game.h"
#include "../engine/clipMap.h"
#include "../engine/dynamicLayer.h"
#include "../engine/tickManager.h"
#include "../engine/utils.h"
#include "../engine/gameTimeManager.h"
#include "cameraEffectTrigger.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

CGatheredResource resSharedCameraEffectsEntityTemplate( TXT("engine\\templates\\camera_effects.w2ent"), 0 );

////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCameraEffectTrigger );
IMPLEMENT_RTTI_ENUM( EEffectEntityPosition );
IMPLEMENT_RTTI_ENUM( EEffectEntityRotation );

CCameraEffectTrigger::CCameraEffectTrigger()
	: m_isPlayingEffect( false )
	, m_useSharedEffects( false )
	, m_isTriggerActivated( false )
	, m_isTicked( false )
	, m_effectEntityPosition( EEP_Camera )
	, m_effectEntityRotation( EER_None )
	, m_effectEntityOffset( 0, 0, 0 )
{
}

void CCameraEffectTrigger::OnAttached( CWorld* world )
{
	m_isTriggerActivated = false;
	m_isTicked = false;
	TBaseClass::OnAttached( world );
}

void CCameraEffectTrigger::OnDetached( CWorld* world )
{
	StopEffect();
	TBaseClass::OnDetached( world );
}

void CCameraEffectTrigger::OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator )
{
	TBaseClass::OnAreaEnter( area, activator );
	StartEffect();
	SetForceNoLOD( true );
	m_isTriggerActivated = true;
	if ( !m_isTicked && ShouldBeTicked() )
	{
		RegisterForTick( true );
	}
}

void CCameraEffectTrigger::OnAreaExit( CTriggerAreaComponent* area, CComponent* activator )
{
	m_isTriggerActivated = false;
	StopEffect();
	SetForceNoLOD( false );
	if ( m_isTicked && !ShouldBeTicked() )
	{
		RegisterForTick( false );
	}
	TBaseClass::OnAreaExit( area, activator );
}

void CCameraEffectTrigger::OnTick( Float timeDelta )
{
	// if trigger was activated AND is affected by day time
	// we need to constantly check if it can/should be played/stopped
	if ( m_isTriggerActivated && IsAffectedByDayTime() )
	{
		if ( m_isPlayingEffect && !CanPlayEffect() )
		{
			StopEffect();
		}
		else if ( !m_isPlayingEffect && CanPlayEffect() )
		{
			StartEffect();
		}
	}
	CEntity* entity = m_effectsEntity.Get();
	if ( entity != nullptr )
	{
		// If we're playing any effect, move the entity to the camera's position
		if ( entity->IsAnyEffectAlive() )
		{
			entity->SetPosition( CalcEntityPosition() );
			if ( m_effectEntityRotation == EER_Continuous )
			{
				RotateEntityForward();
			}
		}
		else // otherwise destroy the entity and remove from ticklist
		{
			entity->Destroy();
			// don't remove from tick manager if affected by daytime
			if ( !ShouldBeTicked() )
			{
				RegisterForTick( false );
			}
		}
	}
}

Vector CCameraEffectTrigger::CalcEntityPosition() const
{
	const Vector cameraPosition = GGame->GetActiveWorld()->GetCameraPosition();
	const Vector playerPosition = GGame->GetPlayerEntity() != nullptr ? GGame->GetPlayerEntity()->GetWorldPosition() : cameraPosition;

	switch ( m_effectEntityPosition )
	{
	case EEP_Player:
		return playerPosition + m_effectEntityOffset;
	case EEP_XYCameraZPlayer:
		return Vector( cameraPosition.X, cameraPosition.Y, playerPosition.Z ) + m_effectEntityOffset;
	case EEP_XYPlayerZCamera:
		return Vector( playerPosition.X, playerPosition.Y, cameraPosition.Z ) + m_effectEntityOffset;
	case EEP_XYCameraZTerrain:
		{
			Float terrainHeight = cameraPosition.Z;
			if ( GGame->GetActiveWorld()->GetTerrain() != nullptr )
			{
				GGame->GetActiveWorld()->GetTerrain()->GetHeightForWorldPosition( cameraPosition, terrainHeight );
			}
			return Vector( cameraPosition.X, cameraPosition.Y, terrainHeight ) + m_effectEntityOffset;
		}
	case EEP_XYPlayerZTerrain:
		{
			Float terrainHeight = playerPosition.Z;
			if ( GGame->GetActiveWorld()->GetTerrain() != nullptr )
			{
				GGame->GetActiveWorld()->GetTerrain()->GetHeightForWorldPosition( playerPosition, terrainHeight );
			}
			return Vector( playerPosition.X, playerPosition.Y, terrainHeight ) + m_effectEntityOffset;
		}
	default: // EEP_Camera
		return cameraPosition + m_effectEntityOffset;
	}
}

void CCameraEffectTrigger::RotateEntityForward()
{
	CEntity* entity = m_effectsEntity.Get();
	if ( entity != nullptr )
	{
		Vector target = GGame->GetActiveWorld()->GetCameraForward();
		target.Z = target.W = 0;
		target.Normalize3();
		Matrix matrix;
		matrix.BuildFromDirectionVector( target );
		EulerAngles rotation = matrix.ToEulerAngles();
		entity->SetRotation( rotation );
	}
}

Bool CCameraEffectTrigger::CanPlayEffect() const
{
	return GGame->IsActive() && m_effectName && CheckDayTime();
}

Bool CCameraEffectTrigger::IsAffectedByDayTime() const
{
	return m_playFrom.m_seconds != 0 || m_playTo.m_seconds != 0;
}

Bool CCameraEffectTrigger::CheckDayTime() const
{
	if ( !IsAffectedByDayTime() )
	{
		return true;
	}
	GameTime time = GGame->GetTimeManager()->GetTime();
	Int32 checkSeconds = time.m_seconds % GameTime::DAY.m_seconds;
	if ( m_playFrom.m_seconds >= m_playTo.m_seconds )
	{
		return checkSeconds >= m_playFrom.m_seconds || checkSeconds < m_playTo.m_seconds;
	}
	else
	{
		return checkSeconds >= m_playFrom.m_seconds && checkSeconds < m_playTo.m_seconds;
	}
}

Bool CCameraEffectTrigger::ShouldBeTicked() const
{
	// we need to tick trigger in two cases:
	// - entity effect is created and needs to be updated
	// - trigger is affected by day time and we need to update its state
	return ( m_effectsEntity.IsValid() || ( IsAffectedByDayTime() && m_isTriggerActivated ) );
}

void CCameraEffectTrigger::RegisterForTick( Bool reg )
{
	if ( reg == m_isTicked )
	{
		return;
	}
	m_isTicked = reg;
	if ( m_isTicked )
	{
		GGame->GetActiveWorld()->GetTickManager()->AddEntity( this );
	}
	else
	{
		GGame->GetActiveWorld()->GetTickManager()->RemoveEntity( this );
	}
}

void CCameraEffectTrigger::StartEffect()
{
	if ( !m_isPlayingEffect && CanPlayEffect() )
	{
		if ( m_useSharedEffects ) // shared entity effect
		{
			// Find the template
			CEntityTemplate* tpl = resSharedCameraEffectsEntityTemplate.LoadAndGet< CEntityTemplate >();
			if ( tpl == nullptr )
			{
				DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("Effects"), TXT("Shared camera effects entity template is missing") );
				m_isPlayingEffect = true;
				return;
			}

			// If there is already an entity created, destroy it
			if ( m_effectsEntity.Get() != nullptr )
			{
				m_effectsEntity.Get()->StopAllEffects();
				m_effectsEntity.Get()->Destroy();
				m_effectsEntity = THandle< CEntity >::Null();
			}

			// Spawn the entity in the dynamic layer
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_template = tpl;
			spawnInfo.m_spawnPosition = CalcEntityPosition();
			CEntity* entity = GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );

			// Start the effect
			if ( entity != nullptr )
			{
				m_effectsEntity = entity;
				if ( m_effectEntityRotation != EER_None )
				{
					RotateEntityForward();
				}
				entity->PlayEffect( m_effectName );

				// Setup ticking that updates the entity's position.
				// If the trigger is affected by day time it was added to tick manager anyway, so no need to do it twice.
				RegisterForTick( true );
			}
		}
		else // camera-specific effect
		{
			CEntity* cameraEnt = SafeCast<CEntity>( GGame->GetActiveWorld()->GetCameraDirector()->GetTopmostCameraObject().Get() );
			if ( cameraEnt )
			{
				cameraEnt->PlayEffect( m_effectName );
			}
		}

		m_isPlayingEffect = true;
	}
}

void CCameraEffectTrigger::StopEffect()
{
	if ( m_isPlayingEffect )
	{
		if ( m_useSharedEffects )
		{
			if ( m_effectsEntity.Get() != nullptr )
			{
				m_effectsEntity.Get()->StopEffect( m_effectName );
			}
		}
		else
		{
			CEntity* cameraEnt = SafeCast<CEntity>( GGame->GetActiveWorld()->GetCameraDirector()->GetTopmostCameraObject().Get() );
			if ( cameraEnt )
			{
				cameraEnt->StopEffect( m_effectName );
			}
		}

		m_isPlayingEffect = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
