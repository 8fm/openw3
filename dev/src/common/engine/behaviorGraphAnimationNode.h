/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorGraphValueNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphAnimationNode;
class CBehaviorGraphAnimationNodeInstance;
class CBehaviorGraphValueNode;
struct CSyncInfo;

class CAnimationSyncToken
{
	DECLARE_RTTI_SIMPLE_CLASS( CAnimationSyncToken );

public:
	virtual ~CAnimationSyncToken(){}
	virtual void Sync( CName animationName, const CSyncInfo& syncInfo, Float weight ) {}
	virtual void Reset() {}
	virtual Bool IsValid() const { return true; }
};

BEGIN_CLASS_RTTI( CAnimationSyncToken );
END_CLASS_RTTI();

class CBehaviorGraphAnimationNode : public CBehaviorGraphValueNode, public IBehaviorGraphProperty
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationNode, CBehaviorGraphValueNode, "Animation", "Animation" );	

protected:
	CName							m_animationName;
	
	Bool							m_loopPlayback;
	Float							m_playbackSpeed;

	Bool							m_applyMotion;
	Bool							m_extractMotionTranslation;
	Bool							m_extractMotionRotation;
	Bool							m_fireLoopEvent;
	CName							m_loopEventName;

	Bool							m_useFovTrack;
	Bool							m_useDofTrack;
	Bool							m_gatherEvents;

	Bool							m_autoFireEffects;
	Bool							m_gatherSyncTokens;

protected:
	TInstanceVar< Float >						i_localTime;
	TInstanceVar< Float >						i_prevTime;
	TInstanceVar< Int32 >						i_loops;
	TInstanceVar< Bool >						i_loopEventFired;
	TInstanceVar< Bool >						i_firstUpdate;
	TInstanceVar< Bool >						i_firstSample;
	TInstanceVar< Bool >						i_effectsFired;
	TInstanceVar< Float >						i_internalBlendTime;
	TInstanceVar< CSkeletalAnimationSetEntry* >	i_animation;
	TInstanceVar< Float >						i_timeDelta;
	TInstanceVar< TDynArray< CAnimationSyncToken* > >	i_syncTokens;
	TInstanceVar< Int32 >						i_setGroupIndex;

protected:
	CBehaviorGraphValueNode*		m_cachedForceTimeNode;
	CBehaviorGraphValueNode*		m_cachedSpeedTimeNode;
	CBehaviorGraphValueNode*		m_cachedForcePropNode;

public:
	struct CBlendBase
	{
		Float  m_time;
		Vector m_position;
	};

	struct SBehaviorGraphAnimEventInfo
	{
		String m_message;
		CName  m_eventName;
		CEntity *m_entity;
		SBehaviorGraphAnimEventInfo() : m_entity(NULL){}
	};

public:
	CBehaviorGraphAnimationNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& /*instance*/, const CBehaviorEvent &/*event*/ ) const { return false; }

	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdateAnimationCache( CBehaviorGraphInstance& instance ) const;

public:
	virtual void CacheConnections();

	//! Get value (current time)
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif

	void SetAnimationName( const CName &name );
	virtual void SetAnimTime( CBehaviorGraphInstance& instance, Float time ) const;

	Float GetAnimTime( CBehaviorGraphInstance& instance ) const;
	Float GetAnimDuration( CBehaviorGraphInstance& instance ) const;
	Float GetAnimProgress( CBehaviorGraphInstance& instance ) const;

	RED_INLINE Bool GetLoopPlayback() const      { return m_loopPlayback; }
	RED_INLINE void SetLoopPlayback( Bool loop ) { m_loopPlayback = loop; }

	RED_INLINE const CName& GetAnimationName() const { return m_animationName; }
	CSkeletalAnimationSetEntry* GetAnimation( CBehaviorGraphInstance& instance ) const;
	virtual void CollectUsedAnimations( TDynArray< CName >& anims ) const;

	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

protected:
	virtual void OnAnimationFinished( CBehaviorGraphInstance& instance ) const;
	virtual void RefreshAnimation( CBehaviorGraphInstance& instance, const CName& animName ) const;
	virtual Bool IsLooped( CBehaviorGraphInstance& instance ) const;
	virtual Bool ApplyMotion( CBehaviorGraphInstance& instance ) const;
	virtual Bool UseFovTrack( CBehaviorGraphInstance& instance ) const;
	virtual Bool UseDofTrack( CBehaviorGraphInstance& instance ) const;
	virtual Float GetPlaybackSpeed( CBehaviorGraphInstance& instance ) const;
	virtual void FirstUpdate( CBehaviorGraphInstance& instance ) const;
	virtual Bool ShouldDoPoseCorrection() const;
	virtual Bool ShouldAddAnimationUsage() const;

	Float GetDuration( CBehaviorGraphInstance& instance ) const;
	void FireLoopEvent( CBehaviorGraphInstance& instance ) const;
	void InternalReset( CBehaviorGraphInstance& instance ) const;
	void SendMissingAnimationEvents( CBehaviorGraphInstance& instance ) const;
	void CollectSyncTokens( CBehaviorGraphInstance& instance ) const;
	void ClearSyncTokens( CBehaviorGraphInstance& instance ) const;

// temp
public:
	Bool SetTempRuntimeAnimationName( CBehaviorGraphInstance& instance, const CName &name ) const;
	void ResetTempRuntimeAnimation( CBehaviorGraphInstance& instance ) const;
	Bool IsTempSlotActive( CBehaviorGraphInstance& instace ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const;
#endif

public:
	virtual void OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const;
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT("Animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_EDIT( m_loopPlayback, TXT("Should animation playback be looped") );
	PROPERTY_EDIT( m_playbackSpeed, TXT("Playback speed (1.0 = normal)") );
	PROPERTY_EDIT( m_applyMotion, TXT("Apply motion from animation to character") );	
	PROPERTY_EDIT( m_extractMotionTranslation, TXT("Extract motion translation from animation") );
	PROPERTY_EDIT( m_extractMotionRotation, TXT("Extract motion rotation from animation") );	
	PROPERTY_EDIT( m_fireLoopEvent, TXT("Fire event at animation loop/finish") );
	PROPERTY_CUSTOM_EDIT( m_loopEventName, TXT("Event to fire at loop/finish"), TXT("BehaviorEventEdition") );
	PROPERTY_EDIT( m_useFovTrack, TXT("Use FOV from animation track") );
	PROPERTY_EDIT( m_useDofTrack, TXT("Use DOF from animation track") );
	PROPERTY_EDIT( m_gatherEvents, TXT("Gather events from animation") );
	PROPERTY_EDIT( m_autoFireEffects, TXT("Fire related effects when playing animation on this node") );
	PROPERTY_EDIT( m_gatherSyncTokens, TXT("Collect tokens for playback sync") );
	PROPERTY( m_cachedForceTimeNode );
	PROPERTY( m_cachedSpeedTimeNode );
	PROPERTY( m_cachedForcePropNode );
END_CLASS_RTTI();

