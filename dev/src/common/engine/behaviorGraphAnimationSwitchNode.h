/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"
#include "Behavior/Tools/bitTools.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphAnimationSwitchNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationSwitchNode, CBehaviorGraphNode, "Blends", "Abs switch" );

	friend class CUndoBehaviorGraphSwitchNodeInput;

protected:
	EInterpolationType						m_interpolation;
	Uint32									m_inputNum;
	Float									m_blendTime;
	Bool									m_synchronizeOnSwitch;
	IBehaviorSyncMethod*					m_syncOnSwitchMethod;

protected:
	TInstanceVar< Int32 >					i_selectInput;
	TInstanceVar< Int32 >					i_prevSelectInput;

	TInstanceVar< Bool >					i_blending;
	TInstanceVar< Float >					i_blendTimer;
	TInstanceVar< Float >					i_blendTimeDuration;

	// This is kind of hack/workaround, for situation where this switch node leaves some inputs active after being deactivated.
	// The error is probably located in CheckActivations(...) method, but:
	// 1. Logic of that function is complicated, I don't want to change anything inside since we are one week before PS4 final build and I don't know the background of hacks over there,
	// 2. Lack of test case - bug is really hard to reproduce (like #107861)
	// After fixing the way this slot is managing its inputs, this mask can be removed.
	// [Note]: this workaround does not solve (hypothetical) situation where given input was activated few times in row by this node, but AFAIK this is not happening.
	TInstanceVar< Uint64 >	i_activatedInputsMask;

protected:
	TDynArray< CBehaviorGraphNode* >		m_cachedInputNodes;
	CBehaviorGraphValueNode*				m_cachedControlValueNode;
	CBehaviorGraphValueNode*				m_cachedBlendTimeValueNode;

public:
	CBehaviorGraphAnimationSwitchNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Switch"); }

#endif

	virtual void CacheConnections();

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated(CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CreateInputSockets()		{}
#endif
	virtual void CacheInputConnections()	{}

	CBehaviorGraphNode* GetSelectInput( CBehaviorGraphInstance& instance ) const;
	CBehaviorGraphNode* GetPrevSelectInput( CBehaviorGraphInstance& instance ) const;

	void CheckActivations( CBehaviorGraphInstance& instance, const Int32 newSelection ) const;

	Bool CanBlend( CBehaviorGraphInstance& instance ) const;
	Bool IsBlending( CBehaviorGraphInstance& instance ) const;
	void StartBlending( CBehaviorGraphInstance& instance ) const;
	void EndBlending( CBehaviorGraphInstance& instance ) const;

	void UpdateBlendTimer( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void CheckBlendingActivation( CBehaviorGraphInstance& instance ) const;

	void ActivateInput( CBehaviorGraphInstance& instance, const Int32 num ) const;
	void DeactivateInput( CBehaviorGraphInstance& instance, const Int32 num ) const;
	Bool IsActiveInput( CBehaviorGraphInstance& instance, const Int32 num ) const;

	void InternalReset( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual CGraphSocket* CreateInputSocket( const CName& name );
	virtual CGraphSocket* CreateOutputSocket( const CName& name );
#endif

	virtual Float GetBlendWeight( CBehaviorGraphInstance& instance ) const;
	virtual CBehaviorGraphNode* CacheInputBlock( const String& name );

	virtual Bool GetPoseType() const;

	virtual Bool ProcessInputSelection( CBehaviorGraphInstance& instance, Float& timeDelta ) const;
	virtual Int32 SelectInput( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float& timeDelta ) const;

	Bool WillSelectedInputFinish( CBehaviorGraphInstance& instance, Float& timeDelta ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationSwitchNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY( m_inputNum );
	PROPERTY( m_cachedInputNodes );
	PROPERTY( m_cachedControlValueNode );
	PROPERTY( m_cachedBlendTimeValueNode );
	PROPERTY_EDIT_RANGE( m_blendTime, TXT("Blend time"), 0.f, FLT_MAX );
	PROPERTY_EDIT( m_interpolation, TXT("Interpolation type") );
	PROPERTY_EDIT( m_synchronizeOnSwitch, TXT("Synchronize child playback on switch") );
	PROPERTY_INLINED( m_syncOnSwitchMethod, TXT("Synchronization method on switch") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationManualSwitchNode : public CBehaviorGraphAnimationSwitchNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationManualSwitchNode, CBehaviorGraphAnimationSwitchNode, "Blends", "Switch" );

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CreateInputSockets();
#endif
	virtual void CacheInputConnections();

public:
	void AddInput();
	void RemoveInput();
	Uint32 GetInputNum() const { return m_inputNum; }

protected:
	void OnInputListChange();
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationManualSwitchNode );
	PARENT_CLASS( CBehaviorGraphAnimationSwitchNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationEnumSwitchNode : public CBehaviorGraphAnimationManualSwitchNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationEnumSwitchNode, CBehaviorGraphAnimationSwitchNode, "Blends", "Switch enum" );

protected:
	CName	m_enum;
	Int32		m_firstInputNum;

public:
	virtual void OnPropertyPostChange( IProperty* property );
	void OnInputListChange();

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CreateInputSockets();
#endif
	virtual void CacheInputConnections();

	CName GetInputName( Uint32 num ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationEnumSwitchNode );
	PARENT_CLASS( CBehaviorGraphAnimationSwitchNode );
	PROPERTY_CUSTOM_EDIT( m_enum, TXT("Enum"), TXT("EnumList") );
	PROPERTY( m_firstInputNum );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationEnumSequentialSwitchNode : public CBehaviorGraphAnimationEnumSwitchNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationEnumSequentialSwitchNode, CBehaviorGraphAnimationEnumSwitchNode, "Blends", "Switch enum sequential" );

protected:
	virtual Bool ProcessInputSelection( CBehaviorGraphInstance& instance, Float& timeDelta ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Sequential switch"); }
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationEnumSequentialSwitchNode );
	PARENT_CLASS( CBehaviorGraphAnimationEnumSwitchNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationRandomSwitchNode : public CBehaviorGraphAnimationManualSwitchNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationRandomSwitchNode, CBehaviorGraphAnimationManualSwitchNode, "Blends", "Random switch" );

	Bool m_randOnlyOnce;

protected:
	CBehaviorGraphAnimationRandomSwitchNode();

	virtual Bool ProcessInputSelection( CBehaviorGraphInstance& instance, Float& timeDelta ) const;
	virtual Int32 SelectInput( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float& timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Random switch"); }
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationRandomSwitchNode );
	PARENT_CLASS( CBehaviorGraphAnimationManualSwitchNode );
	PROPERTY_EDIT( m_randOnlyOnce, TXT("Switch selects node only once during activation") );
END_CLASS_RTTI();
