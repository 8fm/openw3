/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphValueNode;


class CBehaviorGraphConstraintNodeParentAlign	: public CBehaviorGraphBaseNode
												, public IBehaviorGraphBonesPropertyOwner
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintNodeParentAlign, CBehaviorGraphBaseNode, "Constraints", "Parent align" );

protected:
	String									m_bone;
	String									m_parentBone;

	Bool									m_localSpace;

protected:
	TInstanceVar< Int32 >					i_parentBoneIndex;
	TInstanceVar< Int32 >					i_boneIndex;
	TInstanceVar< Float	>					i_controlValue;
	TInstanceVar< TGenericPtr >				i_parentAnimatedComponent; // const CanimatedComponent*

protected:
	CBehaviorGraphValueNode*				m_cachedControlValueNode;

public:
	CBehaviorGraphConstraintNodeParentAlign();

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
	void FindParentAnimatedComponent( CBehaviorGraphInstance& instance ) const;
	const CAnimatedComponent* GetParentAnimatedComponent( CBehaviorGraphInstance& instance ) const;

	void CacheBoneIndex( CBehaviorGraphInstance& instance ) const;
	Bool CheckBones( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintNodeParentAlign );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_parentBone, TXT("Bone for parent skeleton") );
	PROPERTY( m_cachedControlValueNode );
	PROPERTY_EDIT( m_localSpace, TXT("True - local space, false - model space") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphConstraintReset	: public CBehaviorGraphBaseNode
									, public IBehaviorGraphBonesPropertyOwner
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintReset, CBehaviorGraphBaseNode, "Constraints", "Reset" );

protected:
	String					m_bone;

	Bool					m_translation;
	Bool					m_rotation;
	Bool					m_scale;

protected:
	TInstanceVar< Int32 >		i_boneIndex;
	TInstanceVar< Bool >	i_controlValue;

protected:
	CBehaviorGraphValueNode*	m_cachedControlValueNode;

public:
	CBehaviorGraphConstraintReset();

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
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintReset );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_translation, TXT("") );
	PROPERTY_EDIT( m_rotation, TXT("") );
	PROPERTY_EDIT( m_scale, TXT("") );
	PROPERTY( m_cachedControlValueNode );
END_CLASS_RTTI();
