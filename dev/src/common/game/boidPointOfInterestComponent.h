#pragma once

#include "swarmUtils.h"
#include "pointOfInterestParams.h"

class IBoidLairEntity;

enum EZoneAcceptor
{
	ZA_LairAreaOnly,
	ZA_BothTriggersAndLairArea,
};

BEGIN_ENUM_RTTI( EZoneAcceptor )
	ENUM_OPTION( ZA_LairAreaOnly );
	ENUM_OPTION( ZA_BothTriggersAndLairArea );
END_ENUM_RTTI()

/////////////////////////////////////////////////////////////////////////
class CBoidPointOfInterestComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CBoidPointOfInterestComponent, CComponent, 0 );

protected:
	CPointOfInterestParams							m_params;
	EZoneAcceptor									m_acceptor;
	TDynArray< THandle< IBoidLairEntity > >			m_lairs;
	Bool											m_crawlingSwarmDebug;

public:
	CBoidPointOfInterestComponent();

	void ChangePoiTypeFromTool( CName type ){ m_params.m_type = type; }

	void OnAttached( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;
	void AddToLair( IBoidLairEntity* lair );

	virtual void OnUsed( Uint32 count, Float deltaTime );

	const CPointOfInterestParams & GetParameters()const					{ return m_params; }
	EZoneAcceptor GetAcceptor() { return m_acceptor; }

	void funcDisable( CScriptStackFrame& stack, void* result );

#ifndef NO_EDITOR_FRAGMENTS
	// Generate editor rendering fragments
	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;
#endif
};
BEGIN_CLASS_RTTI( CBoidPointOfInterestComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_params.m_type, TXT("Describes the effect the poi has : Fire, Food etc ... ") );
	PROPERTY_EDIT( m_params.m_scale, TXT("Scale, multiplies the parameters found in POIConfig") );
	PROPERTY_EDIT( m_params.m_gravityRangeMin, TXT("The range at which the gravity is equal to gravity") );
	PROPERTY_EDIT( m_params.m_gravityRangeMax, TXT("the range at which the gravity starts to be applied") );
	PROPERTY_EDIT( m_params.m_effectorRadius, TXT("the range the behaviour specific to the POI is triggered, action state or specific effect like fire.") );
	PROPERTY_EDIT( m_acceptor, TXT("The acceptor defines when the lair will see the POI. FALSE: When the POI is in the Lair area zone only, true: When the POI is in the trigger area zones or lair area zone") )
	PROPERTY_EDIT( m_params.m_shapeType, TXT("Type of shape for the poi Circle and Cone are supported") );
	PROPERTY_EDIT( m_params.m_useReachCallBack, TXT("If true LairEntity::OnBoidPointOfInterestReached script will be called for the POI")  );
	PROPERTY_EDIT( m_params.m_closestOnly, TXT("Amongst all POIs with this value set to true, only the closest will affect a given boid")  );
	PROPERTY_EDIT( m_params.m_coneMinOpeningAngle, TXT("(ignored if circle) Angle for the min cone") );
	PROPERTY_EDIT( m_params.m_coneMaxOpeningAngle, TXT("(ignored if circle) Angle for the max cone") );
	PROPERTY_EDIT( m_params.m_coneEffectorOpeningAngle, TXT("(ignored if circle) Angle for the effector cone") );
	PROPERTY_EDIT( m_crawlingSwarmDebug, TXT("Turn this on if you are setting up a 2D swarm spawn point") )
	NATIVE_FUNCTION( "Disable", funcDisable );
END_CLASS_RTTI();




