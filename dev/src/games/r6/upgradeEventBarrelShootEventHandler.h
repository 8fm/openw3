#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;
class CFirearmBarrelDefinitionComponent;

class CBarrelShootEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CBarrelShootEventHandler, CUpgradeEventHandler, 0 );

private:
	void SpawnProjectile( const Vector& position, const Vector& direction, CFirearmBarrelDefinitionComponent* barrel  ) const;
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CBarrelShootEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();

