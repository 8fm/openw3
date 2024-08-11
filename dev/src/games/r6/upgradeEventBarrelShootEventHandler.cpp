#include "build.h"

#include "upgradeEventBarrelShootEventHandler.h"
#include "shootUpgradeEventHandlerParam.h"
#include "itemPartDefinitionComponent.h"
#include "itemPartSlotDefinition.h"
#include "r6damageData.h"
#include "firearmBarrelDefinitionComponent.h"

#include "../../common/engine/PhysicsRagdollWrapper.h"
#include "../../common/game/ProjectileTrajectory.h"
#include "../../common/game/ProjectileTargets.h"
#include "../../common/engine/dynamicLayer.h"


IMPLEMENT_ENGINE_CLASS( CBarrelShootEventHandler );

void CBarrelShootEventHandler::HandleEvent( SUpgradeEventHandlerParam& params )
{
	CItemPartDefinitionComponent* owner = params.m_owner;

	CUpgradeEventHandlerParam* dynamicParams = params.m_dynamicParams;

	if( dynamicParams && dynamicParams->IsA< CShootUpgradeHandlerParam >() )
	{
		CShootUpgradeHandlerParam* sParam = static_cast< CShootUpgradeHandlerParam* >( dynamicParams );
		const EntitySlot* entitySlot = owner->GetEntity()->GetEntityTemplate()->FindSlotByName( CNAME( shoot_position ), true );

		// get start position
		Vector direction = sParam->GetShootDirection();
		Vector startPosition = owner->GetEntity()->GetWorldPosition() + direction;
		if( entitySlot )
		{
			startPosition += entitySlot->GetTransform().GetPosition();
		}

		// has shooting target?
		CEntity* shootingTarget = sParam->GetShootingTarget().Get();
		if( shootingTarget )
		{
			direction = shootingTarget->GetWorldPosition() - params.m_owner->GetEntity()->GetWorldPosition();
			direction.Normalize3();
		}

		// play muzzle flash fx
		owner->GetEntity()->PlayEffect( CNAME( MuzzleFlash ) );

		// spawn bullet
		SpawnProjectile( startPosition, direction, Cast< CFirearmBarrelDefinitionComponent >( owner ) );

		// play sound
		CSoundEmitterComponent* soundEmitterComponent = owner->GetEntity()->GetSoundEmitterComponent();
		if( soundEmitterComponent )
		{
			soundEmitterComponent->SoundEvent( "weapon_rifle_test_shot" );
		}
	}
	
}

void CBarrelShootEventHandler::SpawnProjectile( const Vector& position, const Vector& direction, CFirearmBarrelDefinitionComponent* barrel ) const
{
	if( !barrel )
		return;

	if( !barrel->GetProjectileEntity() )
		return;

	const THandle< CEntityTemplate > projectileTemplate = barrel->GetProjectileEntity();
	
	EntitySpawnInfo einfo;
	einfo.m_template = projectileTemplate;
	einfo.m_spawnPosition = position;
	einfo.m_entityFlags = EF_DestroyableFromScript;
	
	CEntity* entity = GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
	
	CProjectileTrajectory* projectile = Cast< CProjectileTrajectory >( entity );
	
	if( !projectile )
		return;

	projectile->Initialize( barrel->GetEntity() );
	projectile->SetRealCaster( barrel->GetRootEntity() );
    IProjectileTarget* target = new CFixedTarget( position + direction * barrel->GetProjectileRange() );
	projectile->StartProjectile( target, 0, barrel->GetProjectileSpeed(), barrel->GetProjectileRange() );
}
