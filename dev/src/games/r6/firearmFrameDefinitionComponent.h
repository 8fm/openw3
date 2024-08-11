#pragma once


#include "itemPartDefinitionComponent.h"
#include "statsContainerComponent.h"
#include "shootUpgradeEventHandlerParam.h"

class CFirearmFrameDefinitionComponent : public CItemPartDefinitionComponent
{
	DECLARE_ENGINE_CLASS( CFirearmFrameDefinitionComponent, CItemPartDefinitionComponent, 0 );
private:
	THandle< CStatsContainerComponent >		m_weaponStats;	
	Float									m_currentAmmo;
	Float									m_maxAmmo;
	Float									m_reloadTimeLeft;
	Bool									m_isReloading;

	THandle< CShootUpgradeHandlerParam >	m_shootParam;
	Float									m_rateOfFire;
	Float									m_timeSinceLastShoot;	
	Bool									m_isAutoFireOn;

	Bool									m_isTicking;

	void Reload();

	void HandleReload( Float timeDelta );
	void HandleAutoFire( Float timeDelta );

	void StartTicking();
	void StopTicking();

public:		
	RED_INLINE Bool IsMoreAmmo(){ return m_currentAmmo > 0; }
	RED_INLINE Bool IsReloadingFinished(){ return m_reloadTimeLeft <= 0; }
	RED_INLINE Bool IsReloading(){ return m_isReloading; }
	
	RED_INLINE void StartAutoFire( CShootUpgradeHandlerParam* shootParam ){ m_isAutoFireOn = true; m_timeSinceLastShoot = 0; m_shootParam = shootParam; StartTicking(); }
	RED_INLINE void StopAutoFire(){ m_isAutoFireOn = false; m_shootParam = NULL; StopTicking(); }

	Bool ConsumeAmmo( Float ammoToConsume );
	void OnAttachFinished( CWorld* world ) override;
	void StartReloading();	
	void OnTick( Float timeDelta );
};

BEGIN_CLASS_RTTI( CFirearmFrameDefinitionComponent )
	PARENT_CLASS( CItemPartDefinitionComponent );
	PROPERTY( m_shootParam);
	PROPERTY_NAME(  m_currentAmmo, TXT("i_currentAmmo") );
END_CLASS_RTTI();