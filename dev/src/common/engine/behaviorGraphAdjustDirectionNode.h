/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

class CBehaviorGraphAdjustDirectionNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAdjustDirectionNode, CBehaviorGraphBaseNode, "Movement", "Adjust direction" );

protected:
	Float	m_animDirectionChange; //!< Direction change that comes from input animation
	Bool	m_updateAnimDirectionChangeFromAnimation; //!< Update anim direction change from animation (just in case it didn't start where it should)
	Float	m_maxDirectionDiff; //!< How much requested direction may differ from final direction (not adjusted)
	Float	m_maxOppositeDirectionDiff; //!< How much requested direction may differ from final direction when opposite to ignore direction
	CName	m_basedOnEvent; //!< Adjustment happens as long as this event is active
	Bool	m_basedOnEventOverrideAnimation;
	Float	m_adjustmentBlendSpeed; //!< Adjustment blend speed (if changes)
	CName	m_requestedMovementDirectionVariableName; //!< Requested movement direction variable name (used if variable not connected)

protected:
	TInstanceVar< Float	> i_timeDelta;
	TInstanceVar< Bool	> i_firstUpdate;
	TInstanceVar< Float	> i_startingDirectionWS; //!< updated on activation
	TInstanceVar< Float	> i_animDirectionChange; //!< actual anim direction change (in case starting direction has changed)
	TInstanceVar< Float	> i_currentAdjustment; //!< blended based on requested movement direction
	TInstanceVar< Float	> i_eventDuration; //!< if zero, there is no active adjustment
	TInstanceVar< Float	> i_accYaw;
	TInstanceVar< Float > i_accTime;
	// Variables
	TInstanceVar< Bool > i_hasRequestedMovementDirectionVariable;

protected:
	CBehaviorGraphValueNode* m_cachedRequestedMovementDirectionWSValueNode;

public:
	CBehaviorGraphAdjustDirectionNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const override;

protected:
	RED_INLINE Float GetMovementDirectionWS( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAdjustDirectionNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_animDirectionChange, TXT("Direction change that comes from input animation") );
	PROPERTY_EDIT( m_updateAnimDirectionChangeFromAnimation, TXT("Update anim direction change from animation (just in case it didn't start where it should)") );
	PROPERTY_EDIT( m_maxDirectionDiff, TXT("How much requested direction may differ from final direction (not adjusted)") );
	PROPERTY_EDIT( m_maxOppositeDirectionDiff, TXT("How much requested direction may differ from final direction when opposite to ignore direction") );
	PROPERTY_EDIT( m_basedOnEvent, TXT("Adjustment happens as long as this event is active") );
	PROPERTY_EDIT( m_basedOnEventOverrideAnimation, TXT("Adjustment overrides motion from input animation") );
	PROPERTY_EDIT( m_adjustmentBlendSpeed, TXT("Adjustment blend speed (if changes)") );
	PROPERTY_CUSTOM_EDIT( m_requestedMovementDirectionVariableName, TXT("Requested movement direction variable name (used if variable not connected)"), TXT("BehaviorVariableSelection") )
	PROPERTY( m_cachedRequestedMovementDirectionWSValueNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
