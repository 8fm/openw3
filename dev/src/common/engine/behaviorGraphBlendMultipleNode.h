/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphBlendMultipleNode;
class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

class CBehaviorGraphBlendMultipleNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphBlendMultipleNode, CBehaviorGraphNode, "Blends", "Blend multiple" );

	friend class CUndoBehaviorGraphBlendNodeInput;

protected:
	TDynArray< Float >				m_inputValues;

	typedef TPair< Uint32, Float >	tSortedInput;
	TDynArray< tSortedInput >		m_sortedInputs;

	Bool							m_synchronize;
	IBehaviorSyncMethod*			m_syncMethod;

	Float							m_minControlValue;
	Float							m_maxControlValue;
	Bool							m_radialBlending;

	Bool							m_takeEventsFromMoreImportantInput; // Instead of blending events, take them just from more important input

	friend struct Pred;

protected:
	TInstanceVar< Float	>			i_controlValue;
	TInstanceVar< Float >			i_blendWeight;
	TInstanceVar< Float	>			i_minControlValue;
	TInstanceVar< Float	>			i_maxControlValue;
	TInstanceVar< Int32 >			i_selectedInputA;
	TInstanceVar< Int32 >			i_selectedInputB;

protected:
	TDynArray< CBehaviorGraphNode* >	m_cachedInputNodes;
	CBehaviorGraphValueNode*			m_cachedControlValueNode;
	CBehaviorGraphValueNode*			m_cachedMinControlValue;
	CBehaviorGraphValueNode*			m_cachedMaxControlValue;

public:
	CBehaviorGraphBlendMultipleNode();

	virtual void OnSerialize( IFile &file );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *property );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Blend multiple"); }

	Bool ValidateInEditor( TDynArray< String >& outHumanReadableErrorDesc ) const override;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

public:
	void AddInput();
	void RemoveInput( Uint32 index );
	Uint32 GetNumInputs() const;

protected:
	void SelectInputsAndCalcBlendWeight( const CBehaviorGraphInstance& instance, Int32 &firstIndex, Int32 &secondIndex, Float &alpha ) const;
	void SortInputs();
	void OnInputListChange();

	void UpdateControlValue( CBehaviorGraphInstance& instance ) const;
	void ProcessActivations( CBehaviorGraphInstance& instance ) const;

	Int32 GetInputA( const CBehaviorGraphInstance& instance ) const;
	Int32 GetInputB( const CBehaviorGraphInstance& instance ) const;
	Float GetBlendWeight( const CBehaviorGraphInstance& instance ) const;

	void InternalReset( CBehaviorGraphInstance& instance ) const;

public:
	static const Float ACTIVATION_THRESHOLD;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendMultipleNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_synchronize, TXT("Synchronize child playback") );
	PROPERTY_INLINED( m_syncMethod, TXT("Synchronization method") );
	PROPERTY_EDIT( m_inputValues, TXT("Table of variable values representing conseqent inputs") );	
	PROPERTY_EDIT( m_minControlValue, TXT("Minimum value of control value") )
	PROPERTY_EDIT( m_maxControlValue, TXT("Maximum value of control value") )
	PROPERTY_EDIT( m_radialBlending, TXT("Blend between last and first animations (radial blending)") )
	PROPERTY_EDIT( m_takeEventsFromMoreImportantInput, TXT("Instead of blending events, take them just from more important input") )
	PROPERTY( m_cachedInputNodes );
	PROPERTY( m_cachedControlValueNode );
	PROPERTY( m_cachedMinControlValue );
	PROPERTY( m_cachedMaxControlValue );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class IBehaviorGraphBlendMultipleCondNode_Condition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphBlendMultipleCondNode_Condition, CObject );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) {}
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const {}
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const {}

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const {}
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const {}

	virtual void Reset( CBehaviorGraphInstance& instance ) const {}

	virtual void PreUpdate( CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void PostSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const {}

	virtual Bool Check( CBehaviorGraphInstance& instance ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphBlendMultipleCondNode_Condition );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBehaviorGraphBlendMultipleCondNode_Multi : public IBehaviorGraphBlendMultipleCondNode_Condition
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_Multi, IBehaviorGraphBlendMultipleCondNode_Condition, 0 );

private:
	TDynArray< IBehaviorGraphBlendMultipleCondNode_Condition* > m_conditions;
	Bool														m_logicAndOr;

public:
	CBehaviorGraphBlendMultipleCondNode_Multi();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual void PreUpdate( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void PostSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendMultipleCondNode_Multi );
	PARENT_CLASS( IBehaviorGraphBlendMultipleCondNode_Condition );
	PROPERTY_INLINED( m_conditions, String::EMPTY );
	PROPERTY_EDIT( m_logicAndOr, TXT("true = AND, false = OR") );
END_CLASS_RTTI();

class CBehaviorGraphBlendMultipleCondNode_AnimEvent : public IBehaviorGraphBlendMultipleCondNode_Condition
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_AnimEvent, IBehaviorGraphBlendMultipleCondNode_Condition, 0 );

private:
	CName					m_animEventName;

private:
	TInstanceVar< Bool >	i_eventOccured;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual void PostSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendMultipleCondNode_AnimEvent );
	PARENT_CLASS( IBehaviorGraphBlendMultipleCondNode_Condition );
	PROPERTY_EDIT ( m_animEventName, String::EMPTY );
END_CLASS_RTTI();

class CBehaviorGraphBlendMultipleCondNode_AnimEnd : public IBehaviorGraphBlendMultipleCondNode_Condition
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_AnimEnd, IBehaviorGraphBlendMultipleCondNode_Condition, 0 );

public:
	virtual Bool Check( CBehaviorGraphInstance& instance ) const { return false; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendMultipleCondNode_AnimEnd );
	PARENT_CLASS( IBehaviorGraphBlendMultipleCondNode_Condition );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphBlendMultipleCondNode_Transition : public CObject
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_Transition, CObject, 0 );

private:
	Bool					m_synchronize;
	IBehaviorSyncMethod*	m_syncMethod;
	Float					m_transitionDuration;
	Bool					m_blockEvents;
	Bool					m_isEnabled;

	IBehaviorGraphBlendMultipleCondNode_Condition* m_condition;

public:
	CBehaviorGraphBlendMultipleCondNode_Transition();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual void PreUpdate( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void PostSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;

public:
	Float GetTransitionDuration() const	{ return m_transitionDuration; }

	const IBehaviorSyncMethod* GetSyncMethod() const { return m_synchronize ? m_syncMethod : NULL; }

	Bool BlockEvents() const { return m_blockEvents; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendMultipleCondNode_Transition );
	PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_condition, String::EMPTY );
	PROPERTY_EDIT( m_transitionDuration, String::EMPTY );
	PROPERTY_EDIT( m_synchronize, String::EMPTY );
	PROPERTY_INLINED( m_syncMethod, String::EMPTY );
	PROPERTY_EDIT( m_blockEvents, String::EMPTY );
	PROPERTY_EDIT( m_isEnabled, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class IBehaviorGraphBlendMultipleCondNode_DampMethod : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphBlendMultipleCondNode_DampMethod, CObject );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) {}
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const {}
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const {}

	virtual void Reset( CBehaviorGraphInstance& instance, Float value ) const {}

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const {}
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const {}

	virtual void OnUpdate( CBehaviorGraphInstance& instance, Float timeDelta, Float inputRange, Float inputValue, Float& currValue, Float& restTime ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphBlendMultipleCondNode_DampMethod );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBehaviorGraphBlendMultipleCondNode_ConstDampMethod : public IBehaviorGraphBlendMultipleCondNode_DampMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_ConstDampMethod, IBehaviorGraphBlendMultipleCondNode_DampMethod, 0 );

protected:
	Float		m_speed;

public:
	CBehaviorGraphBlendMultipleCondNode_ConstDampMethod();

	virtual void OnUpdate( CBehaviorGraphInstance& instance, Float timeDelta, Float inputRange, Float inputValue, Float& currValue, Float& restTime ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendMultipleCondNode_ConstDampMethod );
	PARENT_CLASS( IBehaviorGraphBlendMultipleCondNode_DampMethod );
	PROPERTY_EDIT( m_speed, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphBlendMultipleCondNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphBlendMultipleCondNode, CBehaviorGraphNode, "Blends", "Blend multiple with transitions" );

	enum EInternalState
	{
		IS_Update_Init,
		IS_UpdateTransition,
		IS_UpdateDamped_InsideArea,
		IS_UpdateDamped_GoToNext,
		IS_UpdateDamped_CheckTransition,
		IS_UpdateDamped_GoStepByStep,
		IS_Update_GoDirectly,
	};

protected:
	TDynArray< Float >					m_inputValues;

	IBehaviorSyncMethod*				m_syncMethodAnimation;

	IBehaviorGraphBlendMultipleCondNode_DampMethod*				m_weightDampMethod;
	IBehaviorGraphBlendMultipleCondNode_DampMethod*				m_controlValueDampMethod;

	TDynArray< CBehaviorGraphBlendMultipleCondNode_Transition* > m_transitions;

	Bool								m_synchronizeAnimations;
	Bool								m_useTransitions;
	Bool								m_useWeightDamp;
	Bool								m_useControlValueDamp;
	Bool								m_radialBlending;

protected:
	TInstanceVar< Float	>				i_controlValue;
	TInstanceVar< Int32 >				i_nodeA;
	TInstanceVar< Int32 >				i_nodeB;
	TInstanceVar< Float >				i_weight;
	TInstanceVar< Int32 >				i_nodeA_T;
	TInstanceVar< Int32 >				i_nodeB_T;
	TInstanceVar< Float >				i_weight_T;
	TInstanceVar< Int32 >				i_transitionIdx;
	TInstanceVar< Float >				i_transitionWeight;

protected:
	TDynArray< CBehaviorGraphNode* >	m_cachedInputNodes;
	CBehaviorGraphValueNode*			m_cachedControlValueNode;

public:
	CBehaviorGraphBlendMultipleCondNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *property );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Blend multiple [trans]"); }

#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

public:
	void AddInput();
	void RemoveInput( Uint32 index );
	Uint32 GetNumInputs() const;

protected:
	void Synchronize( CBehaviorGraphInstance& instance, Float timeDelta, Int32 nodeA, Int32 nodeB, Float weight ) const;
	void SynchronizeTransition( CBehaviorGraphInstance& instance, Int32 firstIndexA, Int32 secondIndexA, Int32 firstIndexB, Int32 secondIndexB, const IBehaviorSyncMethod* syncMethod ) const;

	void OnInputListChange();

	void ProcessActivations( CBehaviorGraphInstance& instance, Int32 prevFirstIndex, Int32 prevSecondIndex, Float prevWeight, Int32 currFirstIndex, Int32 currSecondIndex, Float weight, const IBehaviorSyncMethod* syncMethod ) const;

	Bool IsFirstInputActive( Float var ) const;
	Bool IsSecondInputActive( Float var ) const;

	void FindActiveNodes( CBehaviorGraphInstance& instance, Float controlValue, Int32& nodeA, Int32& nodeB, Float& weight ) const;

	Float FindNearestBoundedWeight( Int32 nodeA, Int32 nodeB, Int32 newNodeA, Int32 newNodeB ) const;
	void FindNearestBoundedWeightAndNodes( Int32 nodeA, Int32 nodeB, Float weight, Int32& newNodeA, Int32& newNodeB, Float& newWeight ) const;
	Float FindNearestBoundedNodesCW( Int32 nodeA, Int32 nodeB, Int32& newNodeA, Int32& newNodeB ) const;
	Float FindNearestBoundedNodesCCW( Int32 nodeA, Int32 nodeB, Int32& newNodeA, Int32& newNodeB ) const;

	void PreUpdateTransitions( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void ResetTransitions( CBehaviorGraphInstance& instance ) const;
	void PostSampleTransitions( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &pose ) const;

	Int32 FindActiveTransition( CBehaviorGraphInstance& instance ) const;
	Bool IsTransitionInProgress( CBehaviorGraphInstance& instance ) const;

	void InternalReset( CBehaviorGraphInstance& instance ) const;

	void InternalUpdateTwoNodes( Int32 nodeA, Int32 nodeB, Float weight, SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void InternalSampleTwoNodes( Int32 nodeA, Int32 nodeB, Float weight, SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	void ProgressNodesWeight( CBehaviorGraphInstance& instance, Float& currWeight, Float destWeight, Float timeDelta, Float inputRange ) const;
	void ProgressNodesBoundedWeight( CBehaviorGraphInstance& instance, Float& currWeight, Float timeDelta, Int32 nodeA, Int32 nodeB, Int32 newNodeA, Int32 newNodeB, Float& restTime ) const;

	Float GetTransitionDuration( CBehaviorGraphInstance& instance, Int32 transitionIdx ) const;
	const IBehaviorSyncMethod* GetTransitionSyncMethod( CBehaviorGraphInstance& instance, Int32 transitionIdx ) const;
	const IBehaviorSyncMethod* GetAnimationSyncMethod() const;
	Bool DoesTransitionBlockEvents( CBehaviorGraphInstance& instance ) const;

	Float GetInputRange( Int32 nodeA, Int32 nodeB ) const;

	static const Float ACTIVATION_THRESHOLD;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendMultipleCondNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_inputValues, TXT("Table of variable values representing conseqent inputs") );
	PROPERTY_EDIT( m_synchronizeAnimations, TXT("Synchronize child playback") );
	PROPERTY_INLINED( m_syncMethodAnimation, TXT("Synchronization method") );
	PROPERTY_EDIT( m_useTransitions, String::EMPTY );
	PROPERTY_INLINED( m_transitions, TXT("Transitions") );
	PROPERTY_EDIT( m_useWeightDamp, String::EMPTY );
	PROPERTY_INLINED( m_weightDampMethod, TXT("Damp for internal weight") );
	PROPERTY_EDIT( m_useControlValueDamp, String::EMPTY );
	PROPERTY_INLINED( m_controlValueDampMethod, TXT("Damp for input control value") );
	PROPERTY_EDIT( m_radialBlending, TXT("Radial blending") );
	PROPERTY( m_cachedInputNodes );
	PROPERTY( m_cachedControlValueNode );
END_CLASS_RTTI();
