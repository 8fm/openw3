/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorConstraintNode.h"

class CBehaviorGraphConstraintNodeCameraFocus : public CBehaviorGraphConstraintNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintNodeCameraFocus, CBehaviorGraphConstraintNode, "Constraints", "Camera Focus" );

protected:
	String					m_bone;
	String					m_bone2;

protected:
	TInstanceVar< Int32 > 	i_boneIndex;
	TInstanceVar< Int32 >		i_boneIndex2;

public:
	CBehaviorGraphConstraintNodeCameraFocus();
	virtual String GetCaption() const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
	virtual AnimQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintNodeCameraFocus );
	PARENT_CLASS( CBehaviorGraphConstraintNode );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_bone2, TXT("Bone2 name"), TXT("BehaviorBoneSelection"));
END_CLASS_RTTI();
