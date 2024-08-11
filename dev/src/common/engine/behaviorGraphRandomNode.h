/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CBehaviorGraphRandomNode;


class CBehaviorGraphRandomNode : public CBehaviorGraphNode
{
	friend class CUndoBehaviorGraphRandomNodeInput;

	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRandomNode, CBehaviorGraphNode, "Misc", "Random" );	

protected:
	TDynArray< Float >		m_weights;
	TDynArray< Float >		m_cooldowns;
	TDynArray< Float >		m_maxStartAnimTime;

protected:
	TInstanceVar< CBehaviorGraphNode* >	i_currentInput;
	TInstanceVar< TDynArray< Float > >	i_currentCooldowns;
	TInstanceVar< Bool >				i_activedInput;

protected:
	TDynArray< CBehaviorGraphNode* >	m_cachedInputNodes;

public:
	CBehaviorGraphRandomNode();;

	virtual void OnPropertyPostChange( IProperty *prop );
	virtual void OnPostLoad();

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

protected:
	virtual void SelectRandomInput( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CreateOutputSocket();
	virtual void CreateInputSocket( const CName& socketName );
#endif
	virtual CBehaviorGraphNode* CacheInputBlock( const String& socketName );

public:
	void AddInput();
	void RemoveInput( Uint32 index );
	Uint32 GetNumInputs() const;

private:
	void OnInputListChange();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRandomNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_weights, TXT("Input weights") );
	PROPERTY_EDIT( m_cooldowns, TXT("Cooldown values") );
	PROPERTY_EDIT( m_maxStartAnimTime, TXT("Max random value of start animation's time in reset") );
	PROPERTY( m_cachedInputNodes );
END_CLASS_RTTI();
