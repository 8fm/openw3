/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiFormationData.h"
#include "behTreeDecoratorSteeringGraph.h"
#include "formationSteeringInput.h"

class CBehTreeNodeDecoratorLeadFormationInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets named party member as sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorLeadFormationDefinition : public CBehTreeDecoratorSteeringGraphDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorLeadFormationDefinition, CBehTreeDecoratorSteeringGraphDefinition, CBehTreeNodeDecoratorLeadFormationInstance, LeadFormation );
protected:
	CBehTreeValFormation			m_formation;
	CBehTreeValBool					m_reshapeOnMoveAction;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorLeadFormationDefinition()
		: m_reshapeOnMoveAction( true )								{}
};


BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorLeadFormationDefinition )
	PARENT_CLASS( CBehTreeDecoratorSteeringGraphDefinition )
	PROPERTY_EDIT( m_formation, TXT("Formation") )
	PROPERTY_EDIT( m_reshapeOnMoveAction, TXT("Rearrange formation whenever we run new move action") )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorLeadFormationInstance : public CBehTreeDecoratorSteeringGraphInstance, public IMovementTargeter, public IMoveLocomotionListener
{
	typedef CBehTreeDecoratorSteeringGraphInstance Super;
protected:
	CFormationLeaderDataPtr			m_leaderDataPtr;
	SFormationSteeringInput			m_steeringInput;
	CFormation*						m_formation;
	Float							m_delayedRearrange;
	Float							m_delayedResume;
	Bool							m_reshapeOnMoveAction;
	Bool							m_isFormationPaused;
public:
	typedef CBehTreeNodeDecoratorLeadFormationDefinition Definition;

	CBehTreeNodeDecoratorLeadFormationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	void				OnDestruction() override;

	Bool				Activate() override;
	void				Deactivate() override;
	void				Update() override;

	Bool				OnEvent( CBehTreeEvent& e ) override;

	void				OnGenerateDebugFragments( CRenderFrame* frame ) override;

	// IMovementTargeter
	void				UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	Bool				IsFinished() const override;

	// IMoveLocomotionListener
	void				OnSegmentPushed( IMoveLocomotionSegment* segment ) override;
};
