/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once


#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphValueNode;


class CBehaviorGraphPivotRotationNode	: public CBehaviorGraphBaseNode
										, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPivotRotationNode, CBehaviorGraphBaseNode, "Misc", "Pivot rotation" );

protected:
	String					m_boneName;
	String					m_pivotBoneName;
	EAxis					m_axis;
	Float					m_scale;
	Float					m_biasAngle;
	Float					m_minAngle;
	Float					m_maxAngle;
	Bool					m_clampRotation;

protected:
	TInstanceVar< Int32 >		i_boneIndex;
	TInstanceVar< Int32 >		i_pivotBoneIndex;
	TInstanceVar< Float >	i_currentAngle;

protected:
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;
	CBehaviorGraphValueNode*	m_cachedAngleMinNode;
	CBehaviorGraphValueNode*	m_cachedAngleMaxNode;

public:
	CBehaviorGraphPivotRotationNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Pivot bone rotation") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

private:
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform GetBoneMS( Int32 bone, const hkQsTransform& parent, const SBehaviorGraphOutput &pose, const CAnimatedComponent* component ) const;
#else
	RedQsTransform GetBoneMS( Int32 bone, const RedQsTransform& parent, const SBehaviorGraphOutput &pose, const CAnimatedComponent* component ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphPivotRotationNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone we want to rotate"), TXT("BehaviorBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_pivotBoneName, TXT("Name of the pivot bone"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_axis, TXT("Rotation axis") );
	PROPERTY_EDIT( m_scale, TXT("Input angle scale") );
	PROPERTY_EDIT( m_biasAngle, TXT("Input angle bias ( applied after scale )") );
	PROPERTY_EDIT( m_minAngle, TXT("Maximum rotation allowed in negative direction") );
	PROPERTY_EDIT( m_maxAngle, TXT("Maximum rotation allowed in positive direction") );
	PROPERTY_EDIT( m_clampRotation, TXT("Clamp rotation") );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedAngleMinNode );
	PROPERTY( m_cachedAngleMaxNode );
END_CLASS_RTTI();
