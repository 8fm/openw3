/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphOutputNode;
class CBehaviorGraphValueNode;

class CBehaviorGraphBlendOverrideNode : public CBehaviorGraphNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphBlendOverrideNode, CBehaviorGraphNode, "Blends", "Blend override" );

protected:
	Bool								m_synchronize;
	IBehaviorSyncMethod*				m_syncMethod;
	Bool								m_synchronizeInputFromParent;
	Bool								m_synchronizeOverrideFromParent;
	IBehaviorSyncMethod*				m_syncMethodFromParent;
	EBehaviorLod						m_lodAtOrAboveLevel;

	TDynArray< SBehaviorGraphBoneInfo >	m_bones;

	Bool								m_alwaysActiveOverrideInput;	// This falg is hack. TODO Remove it!

	Bool								m_getDeltaMotionFromOverride;

protected:
	TInstanceVar< Float	>				i_controlValue;
	TInstanceVar< Float >				i_prevControlValue;

protected:
	CBehaviorGraphNode*					m_cachedInputNode;
	CBehaviorGraphNode*					m_cachedOverrideInputNode;
	CBehaviorGraphValueNode*			m_cachedControlVariableNode;

public:
	CBehaviorGraphBlendOverrideNode();

	virtual TDynArray<SBehaviorGraphBoneInfo>* GetBonesProperty();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Blend override"); }

	virtual Bool WorkWithLod( EBehaviorLod lod ) const;

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

protected:
	void ProcessActivations( CBehaviorGraphInstance& instance ) const;
	void UpdateControlValue( CBehaviorGraphInstance& instance ) const;

	static const Float ACTIVATION_THRESHOLD;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendOverrideNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_synchronize, TXT("Synchronize child input with override playback") );
	PROPERTY_INLINED( m_syncMethod, TXT("Synchronization method") );
	PROPERTY_EDIT( m_synchronizeInputFromParent, TXT("Synchronize input from parent") );
	PROPERTY_EDIT( m_synchronizeOverrideFromParent, TXT("Synchronize override from parent") );
	PROPERTY_INLINED( m_syncMethodFromParent, TXT("Synchronization method from parent") );
	PROPERTY_EDIT( m_lodAtOrAboveLevel, TXT("") );
	PROPERTY_CUSTOM_EDIT_NAME( m_bones, TXT("Bones with weights"), String::EMPTY, TXT("BehaviorBoneMultiSelectionWithWeight") );	
	PROPERTY_EDIT( m_alwaysActiveOverrideInput, TXT("Override input always will be active (CONSULT THIS BEFORE CHECKING)") );
	PROPERTY_EDIT( m_getDeltaMotionFromOverride, TXT("") );
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedOverrideInputNode );
	PROPERTY( m_cachedControlVariableNode );
END_CLASS_RTTI();
