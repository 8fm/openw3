/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behaviorGraphNode.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphRandomSelectNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRandomSelectNode, CBehaviorGraphNode, "Misc", "Random select on activate" );

protected:
	Int32									m_inputNum;
	Bool									m_avoidSelectingPrevious;

protected:
	TInstanceVar< Int32 >					i_selectInput;
	TInstanceVar< Int32 >					i_prevSelectInput;

protected:
	TDynArray< CBehaviorGraphNode* >		m_cachedInputNodes;

public:
	CBehaviorGraphRandomSelectNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Random select"); }
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

public:
	void AddInput();
	void RemoveInput( Uint32 index );
	Uint32 GetNumInputs() const { return m_inputNum; }

private:
	CBehaviorGraphNode* GetSelectInput( CBehaviorGraphInstance& instance ) const;

	void InternalReset( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRandomSelectNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_avoidSelectingPrevious, TXT("When activating avoid selecting previously selected input") );
	PROPERTY( m_cachedInputNodes );
END_CLASS_RTTI();

