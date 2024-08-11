/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/aiPositioning.h"
#include "../../common/game/behTreeCustomMoveData.h"
#include "../../common/game/behTreeDecoratorWalkablePositionQuery.h"
#include "../../common/game/behTreeGuardAreaData.h"

#include "behTreeNodeDecoratorFlight.h"

///////////////////////////////////////////////////////////////////////////////
// Landing decorator meant to handle npc landing mechanism - from availability
// into movement adjustment logic
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeLandingDecoratorInstance;

class CBehTreeNodeLandingDecoratorDefinition : public IBehTreeNodeFlightDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeLandingDecoratorDefinition, IBehTreeNodeFlightDecoratorDefinition, CBehTreeNodeLandingDecoratorInstance, LandingDecorator )
protected:
	CBehTreeValFloat				m_landingMaxHeight;
	CBehTreeValFloat				m_landingForwardOffset;
	CBehTreeValFloat				m_tolerationDistance;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeLandingDecoratorDefinition()
		: m_landingMaxHeight( 4.0f )
		, m_landingForwardOffset( 8.f )
		, m_tolerationDistance( 4.f )											{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeLandingDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeFlightDecoratorDefinition )
	PROPERTY_EDIT( m_landingMaxHeight, TXT("Landing max height") )
	PROPERTY_EDIT( m_landingForwardOffset, TXT("Landing animation max offset") )
	PROPERTY_EDIT( m_tolerationDistance, TXT("Toleration distance") )
END_CLASS_RTTI()

class CBehTreeNodeLandingDecoratorInstance : public IBehTreeNodeFlightDecoratorInstance
{
	typedef IBehTreeNodeFlightDecoratorInstance Super;
protected:
	Float							m_landingMaxHeight;
	Float							m_landingForwardOffset;
	Float							m_tolerationDistance;

	Float							m_spotValidityTimeout;
	Vector3							m_computedSpot;
	Bool							m_spotComputationSuccess;
	Bool							m_isAdjustingPosition;
	Bool							m_landed;

	Bool							ShouldAdjustPosition();
	Bool							ComputeSpot();

	static CName					AnimEventAdjustName();
	static CName					AnimEventLandName();
public:
	typedef CBehTreeNodeLandingDecoratorDefinition Definition;

	CBehTreeNodeLandingDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool							IsAvailable() override;
	Int32							Evaluate() override;

	Bool							Activate() override;

	Bool							OnEvent( CBehTreeEvent& e ) override;

	void							Complete( eTaskOutcome outcome ) override;
};

///////////////////////////////////////////////////////////////////////////////
// Generic take-off decorator
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeTakeOffDecoratorInstance;

class CBehTreeNodeTakeOffDecoratorDefinition : public IBehTreeNodeFlightDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeTakeOffDecoratorDefinition, IBehTreeNodeFlightDecoratorDefinition, CBehTreeNodeTakeOffDecoratorInstance, TakeOffDecorator )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeTakeOffDecoratorDefinition()									{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeTakeOffDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeFlightDecoratorDefinition )
END_CLASS_RTTI()

class CBehTreeNodeTakeOffDecoratorInstance : public IBehTreeNodeFlightDecoratorInstance
{
	typedef IBehTreeNodeFlightDecoratorInstance Super;
protected:
	static CName					AnimEventTakeOffName();

	Bool							m_hadTakeOff;

public:
	typedef CBehTreeNodeTakeOffDecoratorDefinition Definition;

	CBehTreeNodeTakeOffDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_hadTakeOff( false )												{}

	Bool							Activate() override;

	Bool							OnEvent( CBehTreeEvent& e ) override;

	void							Complete( eTaskOutcome outcome ) override;
};

///////////////////////////////////////////////////////////////////////////////
// Find landing spot decorator finds proper landing spot using navigation
// graph nodefinder
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeFindLandingSpotDecoratorInstance;

class CBehTreeNodeFindLandingSpotDecoratorDefinition : public IBehTreeNodeDecoratorWalkableSpotQueryDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeFindLandingSpotDecoratorDefinition, IBehTreeNodeDecoratorWalkableSpotQueryDefinition, CBehTreeNodeFindLandingSpotDecoratorInstance, FindLandingSpot )
protected:
	CBehTreeValFloat				m_desiredLandingSpotDistance;
	CBehTreeValFloat				m_findSpotDistance;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeFindLandingSpotDecoratorDefinition()							{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeFindLandingSpotDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorWalkableSpotQueryDefinition )
END_CLASS_RTTI()


class CBehTreeNodeFindLandingSpotDecoratorInstance : public IBehTreeNodeDecoratorWalkableSpotQueryInstance
{
	typedef IBehTreeNodeDecoratorWalkableSpotQueryInstance Super;
protected:
	CBehTreeGuardAreaDataPtr	m_guardAreaDataPtr;
	Float						m_desiredLandingSpotDistance;
	Float						m_findSpotDistance;

	EQueryStatus				StartQuery() override;

public:
	typedef CBehTreeNodeFindLandingSpotDecoratorDefinition Definition;

	CBehTreeNodeFindLandingSpotDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};