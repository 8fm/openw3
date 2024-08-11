/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../physics/physicalCallbacks.h"
#include "entity.h"

enum EFieldType
{
	FT_Const,
	FT_Linear,
	FT_Square,
};

BEGIN_ENUM_RTTI( EFieldType );
	ENUM_OPTION( FT_Const );
	ENUM_OPTION( FT_Linear );
	ENUM_OPTION( FT_Square );
END_ENUM_RTTI();

class CForceFieldEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CForceFieldEntity, CEntity, 0 );

public:
	static TDynArray< THandle< CForceFieldEntity > > m_elements;

	Float							m_currentForce;

	IFXPhysicalForce*				m_data;
	THandle< CPhantomComponent >	m_phantomComponent;
	THandle< CEntity >				m_parentEntityTemplate;

	CForceFieldEntity() : m_data( nullptr ), m_currentForce( 0.0f ), m_phantomComponent( nullptr ), m_parentEntityTemplate( nullptr ) {}
};

DEFINE_SIMPLE_RTTI_CLASS( CForceFieldEntity, CEntity );

/// Force phantom
class IFXPhysicalForce : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFXPhysicalForce, CObject );

protected:
	TDynArray< String >					m_parameterNames;
	EFieldType							m_fieldType;
	Float								m_radius;
	Bool								m_simulateLocalyInEntity;

public:
	IFXPhysicalForce();

	const TDynArray< String >&		GetParameterNames() const { return m_parameterNames; }
	Uint32							GetParameterCount() const { return m_parameterNames.Size(); }
	
	RED_FORCE_INLINE Float			GetRadius() const { return m_radius; }

	virtual CPhantomComponent*		OnSpawn( CForceFieldEntity* entity, CComponent* component, const CFXTrackItemGlobalSpacePhysicalForce* trackItem, const CFXState& fxState ) const;
	virtual void					OnTick( CForceFieldEntity* entity, const TDynArray< STriggeringInfo >& triggeringInfo, const CFXTrackItemGlobalSpacePhysicalForce* trackItem, const CFXState& fxState, Float timeDelta ) const = 0;
	virtual Vector					Process( CForceFieldEntity* entity, const Vector& targetPosition, float radius ) const { return Vector::ZERO_3D_POINT; }

};


BEGIN_ABSTRACT_CLASS_RTTI( IFXPhysicalForce );
PARENT_CLASS( CObject );
PROPERTY_EDIT( m_fieldType, TXT( "Field type" ) );
PROPERTY_EDIT( m_simulateLocalyInEntity, TXT( "Afect only those components which where placed in efect entity template" ) );

END_CLASS_RTTI();

class CFXExplosionImplosionPhysicalForce : public IFXPhysicalForce
{
	DECLARE_ENGINE_CLASS( CFXExplosionImplosionPhysicalForce, IFXPhysicalForce, 0 );

private:
	Float							m_forceScale;
	Float							m_applyFractureDamage;

public:
	CFXExplosionImplosionPhysicalForce();

	virtual void					OnTick( CForceFieldEntity* entity, const TDynArray< STriggeringInfo >& triggeringInfo, const CFXTrackItemGlobalSpacePhysicalForce* trackItem, const CFXState& fxState, Float timeDelta ) const;
	virtual Vector					Process( CForceFieldEntity* entity, const Vector& targetPosition, float radius ) const;
};

BEGIN_CLASS_RTTI( CFXExplosionImplosionPhysicalForce );
PARENT_CLASS( IFXPhysicalForce );
PROPERTY_EDIT( m_forceScale, TXT( "Force scale" ) );
PROPERTY_EDIT( m_radius, TXT( "Radius" ) );
PROPERTY_EDIT( m_applyFractureDamage, TXT( "Amount of damage in seconds to fracture object" ) );

END_CLASS_RTTI();

class CFXFractureDesctruction : public IFXPhysicalForce
{
	DECLARE_ENGINE_CLASS( CFXFractureDesctruction, IFXPhysicalForce, 0 );

private:

public:
	CFXFractureDesctruction();

	virtual void					OnTick( CForceFieldEntity* entity, const TDynArray< STriggeringInfo >& triggeringInfo, const CFXTrackItemGlobalSpacePhysicalForce* trackItem, const CFXState& fxState, Float timeDelta ) const;

};

BEGIN_CLASS_RTTI( CFXFractureDesctruction );
PARENT_CLASS( IFXPhysicalForce );
PROPERTY_EDIT( m_radius, TXT( "Radius" ) );
END_CLASS_RTTI();
