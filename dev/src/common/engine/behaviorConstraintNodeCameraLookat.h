/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorConstraintNode.h"

class CBehaviorGraphConstraintNodeCameraLookAt : public CBehaviorGraphConstraintNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintNodeCameraLookAt, CBehaviorGraphConstraintNode, "Constraints", "Camera Look at" );

protected:
	String				m_bone;

protected:
	TInstanceVar< Int32 >	i_boneIndex;

public:
	CBehaviorGraphConstraintNodeCameraLookAt();

	virtual String GetCaption() const;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
#ifdef USE_HAVOK_ANIMATION
	virtual hkQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#else
	virtual RedQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintNodeCameraLookAt );
	PARENT_CLASS( CBehaviorGraphConstraintNode );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone name"), TXT("BehaviorBoneSelection"));
END_CLASS_RTTI();
