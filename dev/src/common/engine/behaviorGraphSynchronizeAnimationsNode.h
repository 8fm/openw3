/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behaviorGraphNode.h"
#include "behaviorGraphSimplePlaybackUtils.h"


class CBehaviorGraphValueNode;
class CBehaviorGraphInstance;
struct SBehaviorUsedAnimationDataSet;

///////////////////////////////////////////////////////////////////////////////
//
//	Struct defining names for animation and its additive corresponding "look at" animations
//

struct SSynchronizeAnimationToParentDefinition
{
	DECLARE_RTTI_STRUCT( SSynchronizeAnimationToParentDefinition );

	CName m_parentAnimationName;
	CName m_animationName;

	SSynchronizeAnimationToParentDefinition()
		:	m_parentAnimationName( CName::NONE )
		,	m_animationName( CName::NONE )
	{}

	void CollectUsedAnimations( TDynArray< CName >& anims ) const;
};

BEGIN_CLASS_RTTI( SSynchronizeAnimationToParentDefinition );
	PROPERTY_EDIT_NAME( m_parentAnimationName, TXT("Parent animation"), TXT("Animation played by parent") )
	PROPERTY_CUSTOM_EDIT_NAME( m_animationName, TXT("Play animation"), TXT("Animation to be played"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Runtime instance of pair - contains animations found for this particular entity
//

struct SSynchronizeAnimationToParentInstance
{
	DECLARE_RTTI_STRUCT( SSynchronizeAnimationToParentInstance );

	CName m_parentAnimationName;
	const CSkeletalAnimationSetEntry* m_animation;

	SSynchronizeAnimationToParentInstance()
		:	m_parentAnimationName( CName::NONE )
		,	m_animation( nullptr )
	{}

	SSynchronizeAnimationToParentInstance( CName const & parentAnimationName, const CSkeletalAnimationSetEntry* animation )
		:	m_parentAnimationName( parentAnimationName )
		,	m_animation( animation )
	{}

	void Setup( CBehaviorGraphInstance& instance, const SSynchronizeAnimationToParentDefinition& definition );
};

BEGIN_CLASS_RTTI( SSynchronizeAnimationToParentInstance );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Uses embedded animations. By default it will try to use animation with same name.
//	Currently it synchronizes only to parent entity (used to synchronize riders with horses).
//

class CBehaviorGraphSynchronizeAnimationsToParentNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSynchronizeAnimationsToParentNode, CBehaviorGraphNode, "Synchronization", "Synchronize animations to parent/input" );

protected:
	SSynchronizeAnimationToParentDefinition								m_default;
	TDynArray< SSynchronizeAnimationToParentDefinition >				m_anims;
	Bool																m_syncToInput;
	Bool																m_autoFill;
	Float																m_animationStayMultiplier;
	Bool																m_syncToParentsNormalAnims; // full body
	Bool																m_syncToParentsOverlayAnims;
	Bool																m_skipNormalAnimsForOverlays;
	Bool																m_syncDefaultToAnyLoopedAnim;

protected:
	TInstanceVar< Float >												i_timeDelta;
	TInstanceVar< Float >												i_instanceTimeActive;
	TInstanceVar< SSynchronizeAnimationToParentInstance >				i_default;
	TInstanceVar< TDynArray< SSynchronizeAnimationToParentInstance > >	i_anims;
	TInstanceVar< SSimpleAnimationPlaybackSet >							i_playbacks;
	TInstanceVar< Float >												i_currDefaultTime;
	TInstanceVar< Float >												i_useWeight;
	TInstanceVar< Bool >												i_firstUpdate;

protected:
	CBehaviorGraphNode* m_cachedInputNode;

public:
	CBehaviorGraphSynchronizeAnimationsToParentNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String::Printf( TXT("SyncAnims (to %s)"), m_syncToInput? TXT("input") : TXT("parent") ); }
	virtual void OnRebuildSockets();
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void CollectUsedAnimations( TDynArray< CName >& anims ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;	
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

private:
	RED_INLINE const SSynchronizeAnimationToParentInstance* FindForParents( CBehaviorGraphInstance& instance, const CSkeletalAnimationSetEntry* parentAnimation ) const;

	RED_INLINE Float UseAnimationsInPlaybackSet( CBehaviorGraphInstance& instance, const SBehaviorUsedAnimationDataSet & anims, const SBehaviorUsedAnimationDataSet * notInAnims, SSimpleAnimationPlaybackSet& playbacks, Float timeDelta, Float weightMultiplier ) const;
	RED_INLINE Float UseAnimationInPlaybackSet( CBehaviorGraphInstance& instance, const SBehaviorUsedAnimationData * usedAnim, const CSkeletalAnimationSetEntry * playAnim, SSimpleAnimationPlaybackSet& playbacks, Float timeDelta, Float weightMultiplier ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphSynchronizeAnimationsToParentNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT_NAME( m_syncToInput, TXT("Synchronize to input"), TXT("Synchronize to anims in input node") );
	PROPERTY_EDIT_NAME( m_autoFill, TXT("Auto use anims with same name"), TXT("Auto use animations if they share name") );
	PROPERTY_EDIT_NAME( m_animationStayMultiplier, TXT("Animation stay multiplier"), TXT("How animation's weight will stay to allow smoother blends") );
	PROPERTY_EDIT_NAME( m_syncToParentsNormalAnims, TXT("Sync to normal/full body anims"), TXT("") );
	PROPERTY_EDIT_NAME( m_syncToParentsOverlayAnims, TXT("Sync to overlay body anims"), TXT("") );
	PROPERTY_EDIT_NAME( m_skipNormalAnimsForOverlays, TXT("Skip normal/full body anims when syncing overlay anims"), TXT("") );
	PROPERTY_EDIT_NAME( m_syncDefaultToAnyLoopedAnim, TXT("Sync default to any looped anim found"), TXT("") );
	PROPERTY_EDIT_NAME( m_default, TXT("Default"), TXT("Default animation played when no animation is found") );
	PROPERTY_EDIT_NAME( m_anims, TXT("Anims"), TXT("Pairs of animations that correspond to given animation") );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
