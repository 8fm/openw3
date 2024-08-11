#pragma once

#include "swarmUtils.h"
#include "../engine/areaComponent.h"

/////////////////////////////////////////////////////////////////////////
class CBoidAreaComponent : public CAreaComponent
{
	DECLARE_ENGINE_CLASS( CBoidAreaComponent, CAreaComponent, 0 );
protected:
	Boids::PointOfInterestType						m_boidAreaType;
public:
	CBoidAreaComponent();

	void OnAttached( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;
	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	Boids::PointOfInterestType GetBoidAreaType() const							{ return m_boidAreaType; }
};

BEGIN_CLASS_RTTI( CBoidAreaComponent )
	PARENT_CLASS( CAreaComponent )
	PROPERTY_EDIT( m_boidAreaType, TXT("Type of boid area") )
END_CLASS_RTTI()