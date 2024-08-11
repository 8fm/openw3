/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphValueNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphCameraControllerNode;
class CBehaviorGraphValueNode;

// Camera controller node
class CBehaviorGraphCameraControllerNode	: public CBehaviorGraphBaseNode
											, public IBehaviorGraphBonesPropertyOwner
											, public IBehaviorGraphProperty
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCameraControllerNode, CBehaviorGraphBaseNode, "Controllers", "Camera" );

protected:
	Float					m_currAngle;
	Float					m_valueScale;
	String					m_boneName;
	EAxis					m_axis;
	Bool					m_clamp;
	Float					m_angleMin;
	Float					m_angleMax;

	static const Float		RESET_DURATION;

protected:
	TInstanceVar< Float	>	i_currAngle;
	TInstanceVar< Int32 >		i_boneIndex;
	TInstanceVar< Bool >	i_reset;
	TInstanceVar< Float	>	i_weight;
	TInstanceVar< Int32 >		i_resetEvent;
	TInstanceVar< Int32 >		i_hardResetEvent;
	TInstanceVar< Float >	i_resetTimer;
	TInstanceVar< Float >	i_resetAngle;

protected:
	CBehaviorGraphValueNode*	m_cachedControlInputNode;
	CBehaviorGraphValueNode*	m_cachedWeightInputNode;
	CBehaviorGraphValueNode*	m_cachedControlAngleInputNode;

public:
	CBehaviorGraphCameraControllerNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Camera controller") ); }

#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

protected:
	void NormalizeAngle( Float& angle ) const;
	Bool IsWorking( CBehaviorGraphInstance& instance ) const;
	Float GetAngle( CBehaviorGraphInstance& instance ) const;
	Bool GetDestinationAngle( CBehaviorGraphInstance& instance, Float& angle ) const;
	Bool GetFollowAngle( CBehaviorGraphInstance& instance, Float& angle ) const;
	Float GetResetAngleDuration( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphCameraControllerNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the camera bone"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_valueScale, TXT("Scale for control input") );
	PROPERTY_EDIT( m_axis, TXT("Axis") );
	PROPERTY_EDIT( m_clamp, TXT("Clamp final angle") );
	PROPERTY_EDIT_RANGE( m_angleMin, TXT("Min angle [-180 180]"), -180.f, 180.f );
	PROPERTY_EDIT_RANGE( m_angleMax, TXT("Max angle [-180 180]"), -180.f, 180.f );
	PROPERTY( m_cachedControlInputNode );
	PROPERTY( m_cachedWeightInputNode );
	PROPERTY( m_cachedControlAngleInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphCameraVerticalDampNode	: public CBehaviorGraphVectorValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCameraVerticalDampNode, CBehaviorGraphVectorValueBaseNode, "Controllers", "Camera vertical damp" );

protected:
	Float					m_length;
	Float					m_dampSpeed;

protected:
	TInstanceVar< Float >	i_currZ;
	TInstanceVar< Float >	i_destZ;

public:
	CBehaviorGraphCameraVerticalDampNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Camera vertical damp") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

private:
	Bool IsWorking() const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphCameraVerticalDampNode );
	PARENT_CLASS( CBehaviorGraphVectorValueBaseNode );
	PROPERTY_EDIT( m_length, TXT("Lenght") );
	PROPERTY_EDIT( m_dampSpeed, TXT("Damp speed") );
END_CLASS_RTTI();
