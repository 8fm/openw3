/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "aiQueryWalkableSpotInArea.h"
#include "behTreeGuardAreaData.h"
#include "behTreeNodeAtomicMove.h"


class CBehTreeNodeFleeReactionInstance;

class CBehTreeNodeFleeReactionDefinition : public CBehTreeNodeAtomicMoveToDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeFleeReactionDefinition, CBehTreeNodeAtomicMoveToDefinition, CBehTreeNodeFleeReactionInstance, FleeReaction );
protected:	
	CBehTreeValFloat		m_fleeDistance;
	CBehTreeValFloat		m_surrenderDistance;
	Float					m_queryRadiusRatio;
	Bool					m_useCombatTarget;
public:
	CBehTreeNodeFleeReactionDefinition()
		: m_fleeDistance( 25.f )
		, m_surrenderDistance( 4.f )
		, m_queryRadiusRatio( 0.75f )
		, m_useCombatTarget( false )										{}

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeFleeReactionDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicMoveToDefinition );	
	PROPERTY_EDIT( m_fleeDistance, TXT("Flee distance") );	
	PROPERTY_EDIT( m_surrenderDistance, TXT("Distance at which NPC surrenders and don't try to run away") );
	PROPERTY_EDIT( m_queryRadiusRatio, TXT("Flee distance ratio to search for possible flee point (full flee distance is desired spot, but we can find a node in this distance as a fallback)") );
	PROPERTY_EDIT( m_useCombatTarget, TXT("Action/Combat target") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeFleeReactionInstance : public CBehTreeNodeAtomicMoveToInstance
{

	static const Float					s_updateInterval;	

	typedef CBehTreeNodeAtomicMoveToInstance Super;
protected:
	typedef CQueryReacheableSpotInAreaRequest QueryRequest;

	CBehTreeGuardAreaDataPtr			m_guardAreaDataPtr;
	QueryRequest::Ptr					m_navigationQuery;
	Float								m_fleeDistance;	
	Float								m_fleeDistanceSqrt;
	Float								m_queryRadiusRatio;
	Float								m_surrenderDistance;
	Float								m_nextUpdate;
	Float								m_activationTimeout;

	Bool								m_useCombatTarget;
private:
	CNode* GetTarget();

	void LazyCreateNavigationQuery();
	Bool FindSpot( Bool isAvailabilityTest = false );
	Bool StorePrevData();
	void CompleteWithFailure();
	void CompleteWithSuccess();

protected:
	Bool	ComputeTargetAndHeading() override;

public:

	typedef CBehTreeNodeFleeReactionDefinition Definition;

	CBehTreeNodeFleeReactionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	Bool Activate() override;	
	void Update() override;

	void Complete( eTaskOutcome outcome ) override;

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	//Bool IsInFleeRange();
};