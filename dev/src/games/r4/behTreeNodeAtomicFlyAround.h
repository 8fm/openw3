/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeCustomMoveData.h"
#include "../../common/game/behTreeGuardAreaData.h"

#include "behTreeNodeAtomicFlight.h"

class IBehTreeNodeAtomicFlyAroundBaseInstance;
class CBehTreeNodeAtomicFlyAroundInstance;
class CBehTreeNodeAtomicFlyAroundPositionInstance;


///////////////////////////////////////////////////////////////////////////////
// Base class for fly around behavior
///////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeAtomicFlyAroundBaseDefinition : public IBehTreeNodeAtomicFlightDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeAtomicFlyAroundBaseDefinition, IBehTreeNodeAtomicFlightDefinition, IBehTreeNodeAtomicFlyAroundBaseInstance, FlyAround );
protected:
	Bool					m_stayInGuardArea;
	CBehTreeValFloat		m_distance;
	CBehTreeValFloat		m_distanceMax;
	CBehTreeValFloat		m_height;
	CBehTreeValFloat		m_heightMax;
	CBehTreeValFloat		m_randomizationDelay;
	CBehTreeValFloat		m_pickTargetDistance;
public:
	IBehTreeNodeAtomicFlyAroundBaseDefinition()
		: m_stayInGuardArea( false )
		, m_distance( 20.f )
		, m_distanceMax( -1.f )
		, m_height( 10.f )
		, m_heightMax( -1.f )
		, m_randomizationDelay( 7.5f )
		, m_pickTargetDistance( 10.f )													{}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeAtomicFlyAroundBaseDefinition )
	PARENT_CLASS( IBehTreeNodeAtomicFlightDefinition )
	PROPERTY_EDIT( m_stayInGuardArea, TXT("Fly only in guard area") )
	PROPERTY_EDIT( m_distance, TXT("Desired fly-around 2d distance") )
	PROPERTY_EDIT( m_distanceMax, TXT("Max desired 2d distance (leave -1 to disable randomization)") )
	PROPERTY_EDIT( m_height, TXT( "Desired height difference with target" ) )
	PROPERTY_EDIT( m_heightMax, TXT( "Max desired height (leave -1 to disable randomization)" ) )
	PROPERTY_EDIT( m_randomizationDelay, TXT( "Average delay between randomizations" ) )
	PROPERTY_EDIT( m_pickTargetDistance, TXT( "Distance at which we pick up a target for fly around. For advanced tweaking." ) )
END_CLASS_RTTI()

class IBehTreeNodeAtomicFlyAroundBaseInstance  : public IBehTreeNodeAtomicFlightInstance
{
	typedef IBehTreeNodeAtomicFlightInstance Super;
protected:
	// Data
	Bool						m_isGoingLeft;
	Float						m_distance;
	Float						m_distanceMax;
	Float						m_height;
	Float						m_heightMax;
	Float						m_randomizationDelay;
	Float						m_pickTargetDistance;

	Float						m_desiredDistance;
	Float						m_desiredHeight;
	Float						m_nextRandomizationTimeout;
	Float						m_directionLockTimeout;

	CBehTreeGuardAreaDataPtr	m_guardAreaPtr;

	virtual Bool				GetFlyAroundPosition( Vector& outPosition ) = 0;

public:
	typedef IBehTreeNodeAtomicFlyAroundBaseDefinition Definition;

	IBehTreeNodeAtomicFlyAroundBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool						Activate() override;

	void						Update() override;

	CAreaComponent*				GetAreaEncompassingMovement() override;
};


///////////////////////////////////////////////////////////////////////////////
// Fly around combat/action target
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAtomicFlyAroundDefinition : public IBehTreeNodeAtomicFlyAroundBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicFlyAroundDefinition, IBehTreeNodeAtomicFlyAroundBaseDefinition, CBehTreeNodeAtomicFlyAroundInstance, FlyAroundTarget );
protected:
	Bool					m_useCombatTarget;
public:
	CBehTreeNodeAtomicFlyAroundDefinition()
		: m_useCombatTarget( true )														{}

	IBehTreeNodeInstance*		SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicFlyAroundDefinition )
	PARENT_CLASS( IBehTreeNodeAtomicFlyAroundBaseDefinition )
	PROPERTY_EDIT( m_useCombatTarget, TXT("Use combat or action target") )
END_CLASS_RTTI()

class CBehTreeNodeAtomicFlyAroundInstance  : public IBehTreeNodeAtomicFlyAroundBaseInstance
{
	typedef IBehTreeNodeAtomicFlyAroundBaseInstance Super;
protected:
	// Data
	Bool						m_useCombatTarget;

	Bool						GetFlyAroundPosition( Vector& outPosition ) override;

public:
	typedef CBehTreeNodeAtomicFlyAroundDefinition Definition;

	CBehTreeNodeAtomicFlyAroundInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_useCombatTarget( def.m_useCombatTarget )									{}

};

///////////////////////////////////////////////////////////////////////////////
// Fly around custom position
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAtomicFlyAroundPositionDefinition : public IBehTreeNodeAtomicFlyAroundBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicFlyAroundPositionDefinition, IBehTreeNodeAtomicFlyAroundBaseDefinition, CBehTreeNodeAtomicFlyAroundPositionInstance, FlyAroundPosition );
public:
	CBehTreeNodeAtomicFlyAroundPositionDefinition()										{}

	IBehTreeNodeInstance*		SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicFlyAroundPositionDefinition )
	PARENT_CLASS( IBehTreeNodeAtomicFlyAroundBaseDefinition )
END_CLASS_RTTI()

class CBehTreeNodeAtomicFlyAroundPositionInstance  : public IBehTreeNodeAtomicFlyAroundBaseInstance
{
	typedef IBehTreeNodeAtomicFlyAroundBaseInstance Super;
protected:
	CBehTreeCustomMoveDataPtr	m_customTargetPtr;

	Bool						GetFlyAroundPosition( Vector& outPosition ) override;

public:
	typedef CBehTreeNodeAtomicFlyAroundPositionDefinition Definition;

	CBehTreeNodeAtomicFlyAroundPositionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

};