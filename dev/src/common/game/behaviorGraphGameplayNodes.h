
#pragma once

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphGameplaySoundEventsNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphGameplaySoundEventsNode, CBehaviorGraphBaseNode, "Gameplay", "Sound events" );

protected:
	CName										m_animationName;

protected:
	TInstanceVar< CSkeletalAnimationSetEntry* >	i_animation;
	TInstanceVar< Float >						i_prevTime;
	TInstanceVar< Float >						i_currTime;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Sound events"); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

protected:
	void FindAnimation( CBehaviorGraphInstance& instance ) const;

	void InternalReset( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphGameplaySoundEventsNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT("Animation name"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphRandomAnimTimeNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRandomAnimTimeNode, CBehaviorGraphBaseNode, "Gameplay", "Random anim time" );

protected:
	Float	m_animSpeedMin;
	Float	m_animSpeedMax;
	Float	m_animStartTimeOffset;
	Float	m_animStartTimePrecent;

protected:
	TInstanceVar< Bool > i_firstUpdate;

public:
	CBehaviorGraphRandomAnimTimeNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Random anim time"); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const override;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;

protected:
	void InternalReset( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRandomAnimTimeNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_animSpeedMin, TXT("") );
	PROPERTY_EDIT( m_animSpeedMax, TXT("") );
	PROPERTY_EDIT( m_animStartTimeOffset, TXT("") );
	PROPERTY_EDIT( m_animStartTimePrecent, TXT("m_animStartTimePrecent > 0 means start time will be rand( percent ) * animation duration") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMorphTrackNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMorphTrackNode, CBehaviorGraphBaseNode, "Gameplay", "Morph track" );

	Uint32	m_trackIndex;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Morph Track"); }
#endif

public:
	CBehaviorGraphMorphTrackNode();

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMorphTrackNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_trackIndex, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
