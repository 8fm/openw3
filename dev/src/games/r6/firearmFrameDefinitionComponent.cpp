#include "build.h"

#include "firearmFrameDefinitionComponent.h"
#include "../../common/engine/tickManager.h"

IMPLEMENT_ENGINE_CLASS( CFirearmFrameDefinitionComponent )

void CFirearmFrameDefinitionComponent::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );
	m_weaponStats = GetEntity( )->FindComponent< CStatsContainerComponent >( );	
	m_rateOfFire = 0;

	if( m_weaponStats.Get() )
	{
		m_currentAmmo = m_maxAmmo = m_weaponStats.Get()->GetStatValue( CNAME( MaxAmmo ) );		
		m_rateOfFire = m_weaponStats.Get()->GetStatValue( CNAME( RateOfFire ) );		
		
	}	
	else
	{
		m_currentAmmo = m_maxAmmo = 0;
	}
	if( m_rateOfFire==0 )
		m_rateOfFire = 0.2f;

	m_isTicking = false;
}

void CFirearmFrameDefinitionComponent::Reload()
{
	m_currentAmmo = m_maxAmmo;	
	m_isReloading = false;
}

Bool CFirearmFrameDefinitionComponent::ConsumeAmmo( Float ammoToConsume )
{
	if( m_currentAmmo >= ammoToConsume )
	{
		m_currentAmmo -= ammoToConsume;
		return true;
	}
	return false;
}

void CFirearmFrameDefinitionComponent::StartReloading()
{
	if( m_weaponStats.Get() )
	{
		m_reloadTimeLeft = m_weaponStats.Get()->GetStatValue( CNAME( RealoadTime ) );
		StartTicking();
		m_isReloading = true;
	}		
}
void CFirearmFrameDefinitionComponent::OnTick( Float timeDelta )
{
	HandleReload( timeDelta );
	HandleAutoFire( timeDelta );
}

void CFirearmFrameDefinitionComponent::HandleReload( Float timeDelta )
{
	if( !m_isReloading )
		return;
	m_reloadTimeLeft -= timeDelta;
	if( m_reloadTimeLeft <=0 )
	{
		Reload();
		StopTicking();
	}	
}

void CFirearmFrameDefinitionComponent::HandleAutoFire( Float timeDelta )
{
	if( m_isAutoFireOn )
	{
		if( m_timeSinceLastShoot >= m_rateOfFire )
		{
			if( m_shootParam.Get() )
			{
				ProcessEvent( CNAME( OnShoot ), m_shootParam.Get() );
			}
			m_timeSinceLastShoot = 0;
		}
		else
		{
			m_timeSinceLastShoot += timeDelta;
		}
	}
}

void CFirearmFrameDefinitionComponent::StartTicking()
{
	if( !m_isTicking )
	{
		GetLayer()->GetWorld()->GetTickManager()->AddToGroupDelayed( this, TICK_Main );
		m_isTicking = true;
	}
}

void CFirearmFrameDefinitionComponent::StopTicking()
{
	if( !m_isTicking )
		return;

	if( m_isReloading || m_isAutoFireOn )
		return;

	GetLayer()->GetWorld()->GetTickManager()->RemoveFromGroupDelayed( this, TICK_Main );
	m_isTicking = false;
}
