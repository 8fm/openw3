#pragma once


#include "itemPartDefinitionComponent.h"

class CFirearmBarrelDefinitionComponent : public CItemPartDefinitionComponent
{
	DECLARE_ENGINE_CLASS( CFirearmBarrelDefinitionComponent, CItemPartDefinitionComponent, 0 );

private:
	THandle< CEntityTemplate >	m_projectileEnity;
	Float						m_projectileSpeed;
	Float						m_projectileRange;
public:
	RED_INLINE const THandle< CEntityTemplate >& GetProjectileEntity() const { return m_projectileEnity; }
	RED_INLINE Float GetProjectileSpeed(){ return m_projectileSpeed; }
	RED_INLINE Float GetProjectileRange(){ return m_projectileRange; }
};

BEGIN_CLASS_RTTI( CFirearmBarrelDefinitionComponent )
	PARENT_CLASS( CItemPartDefinitionComponent );	
	PROPERTY_EDIT( m_projectileEnity, TXT("Spawn projectile") );	
	PROPERTY_EDIT( m_projectileSpeed, TXT("Projectile speed") );	
	PROPERTY_EDIT( m_projectileRange, TXT("Projectile range") );	
END_CLASS_RTTI();