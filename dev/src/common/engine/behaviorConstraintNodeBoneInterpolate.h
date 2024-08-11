/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphConstraintNodeBoneInterpolate	: public CBehaviorGraphBaseNode, public IBehaviorGraphBonesPropertyOwner
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintNodeBoneInterpolate, CBehaviorGraphBaseNode, "Constraints", "Bone Interpolate" );

protected:
	String									m_boneInputA;
	String									m_boneInputB;
	String									m_boneOutput;

protected:
	TInstanceVar< Int32 >					i_boneIndexInputA;
	TInstanceVar< Int32 >					i_boneIndexInputB;
	TInstanceVar< Int32	>					i_boneIndexOutput;
	TInstanceVar< Float	>					i_controlValue;

protected:
	CBehaviorGraphValueNode*				m_cachedControlValueNode;

public:
	CBehaviorGraphConstraintNodeBoneInterpolate();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

	virtual void CacheConnections();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const; 

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	void CacheBoneIndex( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintNodeBoneInterpolate );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneInputA, TXT("Bone Input A"), TXT("BehaviorBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_boneInputB, TXT("Bone Input B"), TXT("BehaviorBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_boneOutput, TXT("Bone Output"), TXT("BehaviorBoneSelection") );
	PROPERTY( m_cachedControlValueNode );
END_CLASS_RTTI();