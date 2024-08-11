/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

///////////////////////////////////////////////////////////////////////////////

#include "../engine/behaviorGraphValueNode.h"

///////////////////////////////////////////////////////////////////////////////

/**
 *	Returns blend value for different angles requested
 */
class CBehaviorGraphOnSlopeMovementNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphOnSlopeMovementNode, CBehaviorGraphValueNode, "Movement", "On slope movement (blend value)" );

protected:
	TDynArray< Float > m_angles; // angles for slope values - minus down, plus up
	Float m_slopeBlendTime;
	Float m_slopeMaxBlendSpeed;
	Bool m_neverReachBorderValues; // when we have 3 inputs, central one will always have some weight - this is done to get events from that middle animation

protected:
	TInstanceVar< Matrix > i_prevLocalToWorld;
	TInstanceVar< Float > i_slopeAngle;
	TInstanceVar< Float > i_blendValue;

public:
	CBehaviorGraphOnSlopeMovementNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *property );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("On slope movement (blend value)"); }
#endif

public:
	virtual void CacheConnections();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

private:
	void MakeSureAnglesAreValid();

	RED_INLINE void CalculateBlendValue( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	RED_INLINE Float GetSlopeAngle( CBehaviorGraphInstance& instance, Float timeDelta ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphOnSlopeMovementNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_angles, TXT( "Slope angle values" ) )
	PROPERTY_EDIT( m_slopeBlendTime, TXT( "Facing fir blend time" ) )
	PROPERTY_EDIT( m_slopeMaxBlendSpeed, TXT( "Facing dir max speed time" ) )
	PROPERTY_EDIT( m_neverReachBorderValues, TXT( "When we have 3 inputs, central one will always have some weight - this is done to get events from that middle animation" ) )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
