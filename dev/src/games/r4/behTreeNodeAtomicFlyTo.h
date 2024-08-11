/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeCustomMoveData.h"

#include "behTreeNodeAtomicFlight.h"

class IBehTreeNodeAtomicFlyToBaseInstance;
class CBehTreeNodeAtomicFlyToInstance;
class CBehTreeNodeAtomicFlyToCustomTargetInstance;

///////////////////////////////////////////////////////////////////////////////
// Base class for fly to node
///////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeAtomicFlyToBaseDefinition : public IBehTreeNodeAtomicFlightDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeAtomicFlyToBaseDefinition, IBehTreeNodeAtomicFlightDefinition, IBehTreeNodeAtomicFlyToBaseInstance, FlyTo );
protected:
	Bool						m_skipHeightCheck;
	Bool						m_useAbsoluteHeightDifference;
	Bool						m_checkDistanceWithoutOffsets;
	CBehTreeValBool				m_forceReachDestination;
	CBehTreeValFloat			m_distanceOffset;
	CBehTreeValFloat			m_heightOffset;
	CBehTreeValFloat			m_min2DDistance;
	CBehTreeValFloat			m_minHeight;
public:
	IBehTreeNodeAtomicFlyToBaseDefinition()
		: m_skipHeightCheck( false )
		, m_useAbsoluteHeightDifference( true )
		, m_checkDistanceWithoutOffsets( false )
		, m_forceReachDestination( false )
		, m_distanceOffset( 0.f )
		, m_heightOffset( 1.5f )
		, m_min2DDistance( 2.f )
		, m_minHeight( 1.5f )																	{}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeAtomicFlyToBaseDefinition )
	PARENT_CLASS( IBehTreeNodeAtomicFlightDefinition )
	PROPERTY_EDIT( m_skipHeightCheck, TXT( "Skip height completion test" ) )
	PROPERTY_EDIT( m_useAbsoluteHeightDifference, TXT( "Use absolute height difference" ) )
	PROPERTY_EDIT( m_checkDistanceWithoutOffsets, TXT( "Check distance without takin offsets into account" ) )
	PROPERTY_EDIT( m_distanceOffset, TXT( "2D offset for destination" ) )
	PROPERTY_EDIT( m_heightOffset, TXT( "'Z' offset for destination" ) )
	PROPERTY_EDIT( m_min2DDistance, TXT( "2D distance at which goal completes" ) )
	PROPERTY_EDIT( m_minHeight, TXT("'Z' distance at which goal completes (if 2d distance test is met)") )
END_CLASS_RTTI()

class IBehTreeNodeAtomicFlyToBaseInstance  : public IBehTreeNodeAtomicFlightInstance
{
	typedef IBehTreeNodeAtomicFlightInstance Super;
protected:
	Bool						m_skipHeightCheck;
	Bool						m_useAbsoluteHeightDifference;
	Bool						m_checkDistanceWithoutOffsets;
	Bool						m_forceReachDestination;
	Float						m_distanceOffset;
	Float						m_heightOffset;
	Float						m_min2DDistance;
	Float						m_minHeight;

	virtual Bool				GetFlyToPosition( Vector& outPosition ) = 0;
public:
	typedef IBehTreeNodeAtomicFlyToBaseDefinition Definition;

	IBehTreeNodeAtomicFlyToBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_skipHeightCheck( def.m_skipHeightCheck )
		, m_useAbsoluteHeightDifference( def.m_useAbsoluteHeightDifference )
		, m_checkDistanceWithoutOffsets( def.m_checkDistanceWithoutOffsets )
		, m_forceReachDestination( def.m_forceReachDestination.GetVal( context ) )
		, m_distanceOffset( def.m_distanceOffset.GetVal( context ) )
		, m_heightOffset( def.m_heightOffset.GetVal( context ) )
		, m_min2DDistance( def.m_min2DDistance.GetVal( context ) )
		, m_minHeight( def.m_minHeight.GetVal( context ) )										{}

	Bool						Activate() override;
	void						Update() override;

	void						OnGenerateDebugFragments( CRenderFrame* frame ) override;
};

///////////////////////////////////////////////////////////////////////////////
// Fly to action/combat target
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAtomicFlyToDefinition : public IBehTreeNodeAtomicFlyToBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicFlyToDefinition, IBehTreeNodeAtomicFlyToBaseDefinition, CBehTreeNodeAtomicFlyToInstance, FlyToTarget );
protected:
	Bool						m_useCombatTarget;
public:
	CBehTreeNodeAtomicFlyToDefinition()
		: m_useCombatTarget( true )																{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicFlyToDefinition )
	PARENT_CLASS( IBehTreeNodeAtomicFlyToBaseDefinition )
	PROPERTY_EDIT( m_useCombatTarget, TXT( "Use combat or action target" ) )
END_CLASS_RTTI()



class CBehTreeNodeAtomicFlyToInstance : public IBehTreeNodeAtomicFlyToBaseInstance
{
	typedef IBehTreeNodeAtomicFlyToBaseInstance Super;
protected:
	Bool						m_useCombatTarget;

	virtual Bool				GetFlyToPosition( Vector& outPosition ) override;

public:
	typedef CBehTreeNodeAtomicFlyToDefinition Definition;

	CBehTreeNodeAtomicFlyToInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_useCombatTarget( def.m_useCombatTarget )											{}
};

///////////////////////////////////////////////////////////////////////////////
// Fly to custom target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicFlyToPositionDefinition : public IBehTreeNodeAtomicFlyToBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicFlyToPositionDefinition, IBehTreeNodeAtomicFlyToBaseDefinition, CBehTreeNodeAtomicFlyToCustomTargetInstance, FlyToPosition );
public:
	CBehTreeNodeAtomicFlyToPositionDefinition()											{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicFlyToPositionDefinition )
	PARENT_CLASS( IBehTreeNodeAtomicFlyToBaseDefinition )
END_CLASS_RTTI()



class CBehTreeNodeAtomicFlyToCustomTargetInstance : public IBehTreeNodeAtomicFlyToBaseInstance
{
	typedef IBehTreeNodeAtomicFlyToBaseInstance Super;
protected:
	CBehTreeCustomMoveDataPtr	m_customTargetPtr;

	virtual Bool				GetFlyToPosition( Vector& outPosition ) override;

public:
	typedef CBehTreeNodeAtomicFlyToPositionDefinition Definition;

	CBehTreeNodeAtomicFlyToCustomTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};
