/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "component.h"

class CSimpleBuoyancyComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CSimpleBuoyancyComponent, CComponent, 0 );

public:
	Float					m_waterOffset;
	Float					m_linearDamping;
	Float					m_prevWaterLevelZ;

	Float					m_prevWaterLevelF;
	Float					m_prevWaterLevelB;
	Float					m_prevWaterLevelL;
	Float					m_prevWaterLevelR;
	
	Vector					m_pointFront;
	Vector					m_pointBack;
	Vector					m_pointRight;
	Vector					m_pointLeft;

#ifndef NO_EDITOR
	Bool					m_shouldUpdate;
#endif

public:
	CSimpleBuoyancyComponent();
	~CSimpleBuoyancyComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;
	virtual void OnTickPrePhysics( Float timeDelta ) override;
	virtual void OnSelectionChanged() override;

	virtual void OnPropertyPostChange( IProperty* property ) override;

private:
	Float GetCurrentSmoothValue( Float prevVal, Float waterLvl, Float dt );
	RED_INLINE Float GetWaterOffset() { return m_waterOffset; }
};

BEGIN_CLASS_RTTI( CSimpleBuoyancyComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_waterOffset,			TXT( "Offset for current water level." ) );
	PROPERTY_EDIT_RANGE( m_linearDamping,	TXT( "Linear damping." ), 0.f, 1.f );
	PROPERTY_EDIT( m_pointFront,			TXT( "Buoyancy point offset" ) );
	PROPERTY_EDIT( m_pointBack,				TXT( "Buoyancy point offset" ) );
	PROPERTY_EDIT( m_pointLeft,				TXT( "Buoyancy point offset" ) );
	PROPERTY_EDIT( m_pointRight,			TXT( "Buoyancy point offset" ) );
END_CLASS_RTTI();
