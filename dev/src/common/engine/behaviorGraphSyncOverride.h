
#pragma once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

class CBehaviorGraphSyncOverrideNode : public CBehaviorGraphNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSyncOverrideNode, CBehaviorGraphNode, "Blends", "Blend sync override" );

protected:
	String							m_rootBoneName;
	Bool							m_blendRootParent;
	Float							m_defaultWeight;

protected:
	TInstanceVar< TDynArray< Int32 > > i_bones;
	TInstanceVar< Int32 >				i_boneRoot;
	TInstanceVar< Float	>			i_controlValue;
	TInstanceVar< Float >			i_prevControlValue;

protected:
	CBehaviorGraphNode*				m_cachedInputNode;
	CBehaviorGraphNode*				m_cachedOverrideInputNode;
	CBehaviorGraphValueNode*		m_cachedControlVariableNode;

public:
	CBehaviorGraphSyncOverrideNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Sync override"); }
#endif

public:
	CBehaviorGraphNode* GetBaseInput() const;
	CBehaviorGraphNode* GetOverrideInput() const;
	CBehaviorGraphValueNode* GetControlVariable() const;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

private:
	void UpdateControlValue( CBehaviorGraphInstance& instance ) const;
	Bool IsOverrideActive( CBehaviorGraphInstance& instance ) const;
	void ProcessActivations( CBehaviorGraphInstance& instance ) const;

	RED_INLINE Bool IsActive( Float val ) const { return val > 0.01f; }

	void FillBones( Int32 rootBone, TDynArray< Int32 >& bones, CBehaviorGraphInstance& instance ) const;
	void ConnectTwoPoses( SBehaviorGraphOutput &a, SBehaviorGraphOutput &b, Uint32 bone, CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSyncOverrideNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_CUSTOM_EDIT( m_rootBoneName, TXT("Magic bone name"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_blendRootParent, TXT("") );
	PROPERTY_EDIT( m_defaultWeight, TXT("") );
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedOverrideInputNode );
	PROPERTY( m_cachedControlVariableNode );
END_CLASS_RTTI();

