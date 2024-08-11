/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphFilterNode	: public CBehaviorGraphBaseNode
								, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphFilterNode, CBehaviorGraphBaseNode, "Misc", "Filter transform" );

protected:
	TDynArray< SBehaviorGraphBoneInfo >			m_bones;
	Bool										m_filterTransform;
	Bool										m_filterRotation;
	Bool										m_filterScale;

protected:
	TInstanceVar< Float >						i_masterWeight;

protected:
	CBehaviorGraphValueNode*					m_cachedControlNode;

public:
	CBehaviorGraphFilterNode();

	virtual TDynArray<SBehaviorGraphBoneInfo>* GetBonesProperty();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Filter"); }
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
};

BEGIN_CLASS_RTTI( CBehaviorGraphFilterNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_filterTransform, String::EMPTY );
	PROPERTY_EDIT( m_filterRotation, String::EMPTY );
	PROPERTY_EDIT( m_filterScale, String::EMPTY );
	PROPERTY_CUSTOM_EDIT_NAME( m_bones, TXT("Bones with weights"), String::EMPTY, TXT("BehaviorBoneMultiSelectionWithWeight") );
	PROPERTY( m_cachedControlNode );
END_CLASS_RTTI();
