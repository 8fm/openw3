
#pragma once

class CBehaviorGraphValueNode;
class IBehaviorSyncMethod;

#include "behaviorGraphNode.h"

class CBehaviorGraphBlend3Node : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphBlend3Node, CBehaviorGraphNode, "Blends", "Blend 3" );

protected:
	Bool						m_synchronize;
	IBehaviorSyncMethod*		m_syncMethod;

	Bool						m_useCustomSpace;
	Bool						m_takeEventsFromMostImportantInput; // Instead of blending events, take them from the most important input

	Vector						m_A;
	Vector						m_B;
	Vector						m_C;
	Vector						m_D;

protected:
	TInstanceVar< Float	>		i_controlValue_A;
	TInstanceVar< Float	>		i_prevControlValue_A;
	TInstanceVar< Float	>		i_controlValue_B;
	TInstanceVar< Float	>		i_prevControlValue_B;

protected:
	CBehaviorGraphNode*			m_cachedInputNode_A;
	CBehaviorGraphNode*			m_cachedInputNode_B;
	CBehaviorGraphNode*			m_cachedInputNode_C;
	CBehaviorGraphNode*			m_cachedInputNode_D;
	CBehaviorGraphValueNode*	m_cachedControlVariableNode_A;
	CBehaviorGraphValueNode*	m_cachedControlVariableNode_B;

public:
	CBehaviorGraphBlend3Node();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Blend 3"); }

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

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();

protected:
	void GetWeights3( CBehaviorGraphInstance& instance, Float weights[3] ) const;
	void GetWeights3( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC ) const;

	void GetPrevWeights3( CBehaviorGraphInstance& instance, Float weights[3] ) const;
	void GetPrevWeights3( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC ) const;

	void GetWeights4( CBehaviorGraphInstance& instance, Float weights[4] ) const;
	void GetWeights4( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC, Float& weightD ) const;

	void GetPrevWeights4( CBehaviorGraphInstance& instance, Float weights[4] ) const;
	void GetPrevWeights4( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC, Float& weightD ) const;

	void CalcWeight4( Float a, Float b, Float& weightA, Float& weightB, Float& weightC, Float& weightD ) const;

	void GetNodes3( const CBehaviorGraphNode* nodes[3] ) const;
	void GetNodes4( const CBehaviorGraphNode* nodes[4] ) const;

	Int32 GetBestMainNodeIndex( CBehaviorGraphInstance& instance ) const;
	const CBehaviorGraphNode* GetBestMainNode( CBehaviorGraphInstance& instance ) const;

	Int32 GetBestPrevMainNodeIndex( CBehaviorGraphInstance& instance ) const;
	const CBehaviorGraphNode* GetBestPrevMainNode( CBehaviorGraphInstance& instance ) const;

	void ProcessActivations( CBehaviorGraphInstance& instance ) const;
	void UpdateControlValues( CBehaviorGraphInstance& instance ) const;

	Bool IsInputActive( Float var ) const;

	static const Float ACTIVATION_THRESHOLD;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlend3Node );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_synchronize, TXT("Synchronize child playback") );
	PROPERTY_INLINED( m_syncMethod, TXT("Synchronization method") );
	PROPERTY_EDIT( m_useCustomSpace, TXT("") );
	PROPERTY_EDIT( m_takeEventsFromMostImportantInput, TXT("Instead of blending events, take them from the most important input") );
	PROPERTY_EDIT( m_A, TXT("") );
	PROPERTY_EDIT( m_B, TXT("") );
	PROPERTY_EDIT( m_C, TXT("") );
	PROPERTY_EDIT( m_D, TXT("") );
	PROPERTY( m_cachedInputNode_A );
	PROPERTY( m_cachedInputNode_B );
	PROPERTY( m_cachedInputNode_C );
	PROPERTY( m_cachedInputNode_D );
	PROPERTY( m_cachedControlVariableNode_A );
	PROPERTY( m_cachedControlVariableNode_B );
END_CLASS_RTTI();
