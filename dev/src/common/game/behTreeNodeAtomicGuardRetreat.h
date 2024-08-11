/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "aiQueryWalkableSpotInArea.h"
#include "behTreeCustomMoveData.h"
#include "behTreeGuardAreaData.h"
#include "behTreeNodeSetupCustomMoveData.h"


class CBehTreeNodeDecoratorGuardRetreatInstance;


class CBehTreeNodeDecoratorGuardRetreatDefinition : public IBehTreeNodeSetupCustomMoveDataDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorGuardRetreatDefinition, IBehTreeNodeSetupCustomMoveDataDefinition, CBehTreeNodeDecoratorGuardRetreatInstance, GuardRetreat );

protected:
	Bool										m_isAvailableWhenInPursuitRange;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const;
public:
	CBehTreeNodeDecoratorGuardRetreatDefinition()
		: m_isAvailableWhenInPursuitRange( false )							{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorGuardRetreatDefinition )
	PARENT_CLASS( IBehTreeNodeSetupCustomMoveDataDefinition)
	PROPERTY_EDIT( m_isAvailableWhenInPursuitRange, TXT("Allow pursue into pursue area/range (whichever provided)") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDecoratorGuardRetreatInstance : public IBehTreeNodeSetupCustomMoveDataInstance
{
	typedef IBehTreeNodeSetupCustomMoveDataInstance Super;
protected:
	CBehTreeGuardAreaDataPtr					m_guardAreaPtr;
	CBehTreeCustomMoveDataPtr					m_customMoveDataPtr;
	CQueryReacheableSpotInAreaRequest::Ptr		m_retreatRequest;
	Float										m_delayReactivation;
	Bool										m_isAvailableWhenInPursuitRange;

	Bool										ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) override;
	void										LazyCreateRequest();

	Bool										CheckCondition();
	Bool										FindSpot();
public:

	typedef CBehTreeNodeDecoratorGuardRetreatDefinition Definition;

	CBehTreeNodeDecoratorGuardRetreatInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	
	////////////////////////////////////////////////////////////////////
	// Base interface
	Int32										Evaluate() override;
	Bool										IsAvailable() override;
	Bool										Activate() override;
	void										Deactivate() override;
	Bool										OnListenedEvent( CBehTreeEvent& e ) override;

	void										Complete( eTaskOutcome outcome ) override;
	void										OnDestruction() override;
};