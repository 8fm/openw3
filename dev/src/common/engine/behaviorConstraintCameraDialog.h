/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphConstraintCameraDialog	: public CBehaviorGraphBaseNode
											, public IBehaviorGraphBonesPropertyOwner
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintCameraDialog, CBehaviorGraphBaseNode, "Constraints", "Camera Dialog" );

protected:
	String					m_cameraBone;
	Float					m_referenceZ;

protected:
	TInstanceVar< Int32 >		i_boneIndex;
	TInstanceVar< Bool >	i_controlValue;
	TInstanceVar< Matrix >	i_boneTransform;
	TInstanceVar< Vector >	i_sourceMS;
	TInstanceVar< Vector >	i_destMS;

protected:
	CBehaviorGraphVectorValueNode*	m_cachedSourceTargetValueNode;
	CBehaviorGraphVectorValueNode*	m_cachedDestTargetValueNode;
	CBehaviorGraphValueNode*		m_cachedControlValueNode;

public:
	CBehaviorGraphConstraintCameraDialog();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

	virtual void CacheConnections();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

protected:
	Bool IsRunning( CBehaviorGraphInstance& instance ) const;
	Matrix BuildSpecialMatrixFromXDirVector( const Vector& xVec ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintCameraDialog );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_cameraBone, TXT("Camera bone"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_referenceZ, TXT("") );
	PROPERTY( m_cachedControlValueNode );
	PROPERTY( m_cachedSourceTargetValueNode );
	PROPERTY( m_cachedDestTargetValueNode );
END_CLASS_RTTI();

