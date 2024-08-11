/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphAnimationSlotNode.h"
#include "behaviorGraphMimicsAnimationNode.h"
#include "behaviorGraphTransitionBlend.h"
#include "../engine/slotAnimationShiftingInterval.h"

class CBehaviorGraphAnimationBaseSlotNode;

// --- To jest jakis krap - do przepisania!
class ISlotAnimationListener
{
public:
	enum EStatus
	{
		S_Finished,
		S_Stopped,
		S_Deactivated,
		S_BlendOutStarted,
	};

	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status ) = 0;
	virtual String GetListenerName() const = 0;
};
// --- 

struct SBehaviorSlotSetup
{
	SBehaviorSlotSetup();

	Bool							m_looped;			//!< Is animation looped
	Bool							m_motionEx;			//!< Use motion extraction
	Float							m_speedMul;			//!< Animation speed multiplier
	Float							m_offset;			//!< Animation starting offset [0,1), start time = m_offset * duration

	//++ TODO:
	Float							m_weight;			//!< Animation weight factor
	//--

	Float							m_blendIn;			//!< Blend in duration
	Float							m_blendOut;			//!< Blend out duration

	EBehaviorTransitionBlendMotion	m_blendInType;		//!< Blend in type
	EBehaviorTransitionBlendMotion	m_blendOutType;		//!< Blend out type

	Bool							m_mergeBlendedSlotEvents;	//!< Merge events if animation is blended (in/out)

	Bool							m_useFovTrack;		//!< Use fov track ( camera )
	Bool							m_useDofTrack;		//!< Use dof tracks ( camera )

	ISlotAnimationListener*			m_listener;			//!< Animation slot listener

	const TDynArray<CSlotAnimationShiftingInterval>* m_animationShifts;
};

#ifndef NO_EDITOR_GRAPH_SUPPORT
class CAnimatedComponentAnimationSyncToken : public CAnimationSyncToken
{
	DECLARE_RTTI_SIMPLE_CLASS( CAnimatedComponentAnimationSyncToken );

	THandle< CAnimatedComponent >	m_syncedAnimated;		//!< Animated component to be synced through token

public:
	CAnimatedComponentAnimationSyncToken();
	virtual void Sync( CName animationName, const CSyncInfo& syncInfo, Float weight ) override;
	virtual void Reset() override;
	virtual Bool IsValid() const override;
};

BEGIN_CLASS_RTTI( CAnimatedComponentAnimationSyncToken );
END_CLASS_RTTI();
#endif

class CBehaviorGraphAnimationBaseSlotNode : public CBehaviorGraphMimicsAnimationNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationBaseSlotNode, CBehaviorGraphMimicsAnimationNode, "Slots.Animation", "Base" );	

protected:
	struct SSlotAnimationShift
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

		Vector										m_animationShift;
		TDynArray< CSlotAnimationShiftingInterval >	m_animationShiftingIntervals;
	};

protected:
	CName					m_slotName;
	String					m_animPrefix;
	String					m_animSufix;

protected:
	TInstanceVar< Bool >	i_motionEx;
	TInstanceVar< Bool >	i_looped;
	TInstanceVar< Bool >	i_useFovTrack;
	TInstanceVar< Bool >	i_useDofTrack;
	TInstanceVar< Bool >	i_animationPaused;
	TInstanceVar< Float >	i_speed;
	TInstanceVar< Float >	i_offset;
	TInstanceVar< Bool >	i_needToRefreshSyncTokens;

	TInstanceVar< TGenericPtr >	i_slotAnimationListener;
	TInstanceVar< TGenericPtr >	i_slotAnimationShift;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
	virtual Color GetTitleColor() const;
#endif

public:
	virtual void OnSerialize( IFile &file );
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool IsMimic() const { return false; }

public:
	virtual Bool IsSlotActive( const CBehaviorGraphInstance& instance ) const;
	virtual Bool IsPlayingSlotAnimation( const CBehaviorGraphInstance& instance ) const;

	virtual void PauseAnimation( CBehaviorGraphInstance& instance, const Bool& pause  ) const;
	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, const CName& animation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime = 0.0f ) const;
	Bool DetachListener( CBehaviorGraphInstance& instance, ISlotAnimationListener* listener ) const;
	Bool HasListener( const CBehaviorGraphInstance& instance, ISlotAnimationListener* listener ) const;

	const CName& GetSlotName() const;

	void SetNeedRefreshSyncTokens( CBehaviorGraphInstance& instance, Bool value ) const;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void AppendSyncTokenForEntity( CBehaviorGraphInstance& instance, const CEntity* entity ) const;
#endif

protected:
	virtual Bool IsValid( CBehaviorGraphInstance& instance ) const;
	virtual void SlotReset( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void OnAnimationFinished( CBehaviorGraphInstance& instance ) const;
	virtual void SetupSlot( CBehaviorGraphInstance& instance, const SBehaviorSlotSetup* setup ) const;

	virtual Bool IsLooped( CBehaviorGraphInstance& instance ) const;
	virtual Bool ApplyMotion( CBehaviorGraphInstance& instance ) const;
	virtual Bool UseFovTrack( CBehaviorGraphInstance& instance ) const;
	virtual Bool UseDofTrack( CBehaviorGraphInstance& instance ) const;
	virtual Float GetPlaybackSpeed( CBehaviorGraphInstance& instance ) const;
	virtual void FirstUpdate( CBehaviorGraphInstance& instance ) const;

	void SetSlotAnimationListener( CBehaviorGraphInstance& instance, ISlotAnimationListener* listener ) const;

	SSlotAnimationShift* GetSlotShift( CBehaviorGraphInstance& instance ) const;
	void UpdateSlotShifts( CBehaviorGraphInstance& instance, Float timeDelta ) const;

	Bool SetRuntimeAnimationByName( CBehaviorGraphInstance& instance, const CName &name ) const;
	Bool SetRuntimeAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* anim ) const;

	void ApplyAnimOffset( CBehaviorGraphInstance& instance ) const;

	CName GetAnimationFullName( const CName& animation ) const;

public:
	virtual void OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const;
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationBaseSlotNode );
	PARENT_CLASS( CBehaviorGraphMimicsAnimationNode );
	PROPERTY_RO( m_slotName, TXT("Slot name - use for play slot animation") );
	PROPERTY_EDIT( m_animPrefix, TXT("Add prefix to all slot animation names") );
	PROPERTY_EDIT( m_animSufix, TXT("Add sufix to all slot animation names") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicBaseSlotNode : public CBehaviorGraphAnimationBaseSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicBaseSlotNode, CBehaviorGraphAnimationBaseSlotNode, "Slots.Mimic", "Base" );

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual Bool IsMimic() const { return true; }

protected:
	virtual Bool IsValid( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicBaseSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationBaseSlotNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class IBehaviorGraphSlotInterface
{
	THandle< CBehaviorGraphInstance >		m_instance;
	CBehaviorGraphAnimationBaseSlotNode*	m_slot;

public:
	IBehaviorGraphSlotInterface();

	void Init( CBehaviorGraphAnimationBaseSlotNode* slot, CBehaviorGraphInstance* instance );
	Bool IsValid() const;

	Bool IsActive() const;

	Bool PlayAnimation( const CName& animation, const SBehaviorSlotSetup* slotSetup = NULL );
	Bool PlayAnimation( CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup = NULL );
	void StopAnimation();

	void GetSyncInfo( CSyncInfo& info ) const;
	void SynchronizeTo( const CSyncInfo& info );

	const CName& GetAnimationName() const;
	CSkeletalAnimationSetEntry* GetAnimation() const;

	Bool GetSlotPose( SBehaviorGraphOutput& pose ) const;
	Bool GetSlotCompressedPose( SBehaviorGraphOutput& pose ) const;

private:
	Bool IsValid( CBehaviorGraphInstance *& instance ) const;
};
