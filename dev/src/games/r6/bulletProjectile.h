#pragma once

#include "../../common/game/ProjectileTrajectory.h"

//------------------------------------------------------------------------------------------------------------------
// Temp way of doing bullet / physics material effects... 
// Should preferably be added to the physics material at some point.
//------------------------------------------------------------------------------------------------------------------
struct SBulletImpactInfo
{
	DECLARE_RTTI_STRUCT( SBulletImpactInfo );

	CName			m_materialName;
	CName			m_effectName;
};

BEGIN_CLASS_RTTI( SBulletImpactInfo );
	PROPERTY_EDIT_NAME( m_materialName,	TXT("Material Name"), TXT("") );
	PROPERTY_EDIT_NAME( m_effectName,	TXT("Effect Name"), TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CBulletProjectile : public CProjectileTrajectory
{
	DECLARE_ENGINE_CLASS( CBulletProjectile, CProjectileTrajectory, 0 );

protected:
	TDynArray< SBulletImpactInfo >	m_impactInfos;

public:
	virtual void OnTick( Float timeDelta );

protected:
	//virtual void CollisionCheck( const Vector& startPoint, const Vector& endPoint, Float timeDelta );

#ifndef RED_FINAL_BUILD
	static Int32 sm_debugBulletIndex;
#endif
};

BEGIN_CLASS_RTTI( CBulletProjectile );
	PARENT_CLASS( CProjectileTrajectory );
	PROPERTY_EDIT_NAME( m_impactInfos,	TXT("Impact Effect Look Ups"), TXT("") );	
END_CLASS_RTTI();
