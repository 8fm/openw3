/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphValueNode;

///////////////////////////////////////////////////////////////////////////////

#include "behaviorGraphValueNode.h"
#include "behaviorGraphSimplePlaybackUtils.h"
#include "behaviorIncludes.h"
#include "../core/instanceVar.h"

///////////////////////////////////////////////////////////////////////////////
//
//	Contains calculations for looking at point in space and enabling/disabling look at
//

class CBehaviorGraphLookAtUsingAnimationsProcessingNode : public CBehaviorGraphVectorValueNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphLookAtUsingAnimationsProcessingNode, CBehaviorGraphVectorValueNode );

protected:
	Vector2							m_angleLimit;
	Vector2							m_animationAngleRange;
	Float							m_angleToStopLooking;
	Float							m_lookAtSpeed;
	Float							m_lookAtBlendTime;
	Float							m_controlValueBlendTime;
	Float							m_defaultControlValue;
	CName							m_bone;
	Bool							m_useCharacterRot;

protected:
	TInstanceVar< Int32	>			i_boneIdx;
	TInstanceVar< Float >			i_controlValue;
	TInstanceVar< Vector >			i_lookAtTargetDirMS;
	TInstanceVar< Vector2 >			i_lookAt;
	TInstanceVar< Float >			i_storedTimeDelta;
	TInstanceVar< Bool >			i_isLooking;
	TInstanceVar< Bool >			i_justActivated;
	TInstanceVar< Float >			i_eventBlockingControlValue;
	TInstanceVar< EAxis	>			i_fwdAxis;

protected:
	CBehaviorGraphNode*				m_cachedInputNode;
	CBehaviorGraphVectorValueNode*	m_cachedLookAtVariableNode;
	CBehaviorGraphValueNode*		m_cachedControlVariableNode;
	CBehaviorGraphValueNode*		m_cachedLookAtBlendTimeNode;

public:
	CBehaviorGraphLookAtUsingAnimationsProcessingNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Look at (anim based, processing)"); }
#endif

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! get title color
	virtual Color GetTitleColor() const { return CBehaviorGraphNode::GetTitleColor(); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void UpdateControlValue( CBehaviorGraphInstance& instance, Float timeDelta, Bool dontBlend = false ) const;
	void UpdateEventBlockingControlValue( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;

	void UpdateLookAt( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput * output ) const;

	RED_INLINE Float GetLookAtBlendTime( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphLookAtUsingAnimationsProcessingNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
	PROPERTY_EDIT_NAME( m_bone, TXT("Bone name"), TXT("Bone that is used as 'look at' reference point") );
	PROPERTY_EDIT_NAME( m_useCharacterRot, TXT("Use char. rotation"), TXT("Use character rotation as ref point for calculating look at (instead of bone rot)") );
	PROPERTY_EDIT_NAME( m_angleLimit, TXT("Angle limit"), TXT("'Look at' angle limit. Character can't turn bone more than this") );
	PROPERTY_EDIT_NAME( m_animationAngleRange, TXT("Animation angle range"), TXT("'Look at' animation range. How far can character turn") );
	PROPERTY_EDIT_NAME( m_angleToStopLooking, TXT("Angle to stop looking"), TXT("Angle at which character stops looking at its target") );
	PROPERTY_EDIT_NAME( m_lookAtSpeed, TXT("Max 'Look at' speed"), TXT("Max speed for looking at") );
	PROPERTY_EDIT_NAME( m_lookAtBlendTime, TXT("'Look at' blend time"), TXT("Blend time for looking at") );
	PROPERTY_EDIT_NAME( m_controlValueBlendTime, TXT("On//Off blend time"), TXT("Blend time for switching on and off") );
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedLookAtVariableNode );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedLookAtBlendTimeNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Contains information about blending (type, which comes first, alternative mapping
//

class CBehaviorGraphLookAtUsingAnimationsCommonBaseNode : public CBehaviorGraphLookAtUsingAnimationsProcessingNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphLookAtUsingAnimationsCommonBaseNode, CBehaviorGraphLookAtUsingAnimationsProcessingNode );

protected:
	EAdditiveType					m_type;
	Bool							m_horizontalFirst;
	Bool							m_alternativeMapping;

public:
	CBehaviorGraphLookAtUsingAnimationsCommonBaseNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Look at (anim based, common base)"); }
#endif

	RED_INLINE Float MapAdditiveValue( Float val ) const;

	void BlendVerticalAndHorizontalPoses( SBehaviorGraphOutput &output, Float controlValue, SBehaviorGraphOutput* vertical, SBehaviorGraphOutput* horizontal ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphLookAtUsingAnimationsCommonBaseNode );
	PARENT_CLASS( CBehaviorGraphLookAtUsingAnimationsProcessingNode );
	PROPERTY_EDIT_NAME( m_type, TXT("Additive blend type"), TXT("") );
	PROPERTY_EDIT_NAME( m_horizontalFirst, TXT("Horizontal blend is first"), TXT("Choose if vertical or horizontal blend is done first. 'Horizontal first' gives more natural results in most cases") );
	PROPERTY_EDIT_NAME( m_alternativeMapping, TXT("Alternative additive mapping"), TXT("Use mapping 1 -> 3 seconds instead of patched 1 -> 0 and 2 -> 3") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Uses input sockets to connect additive animations to be used for look at
//

class CBehaviorGraphLookAtUsingAnimationsNode : public CBehaviorGraphLookAtUsingAnimationsCommonBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLookAtUsingAnimationsNode, CBehaviorGraphLookAtUsingAnimationsCommonBaseNode, "Look ats", "Look at (animation based)" );

protected:
	// ??

protected:
	CBehaviorGraphNode*				m_cachedVerticalAdditiveInputNode;
	CBehaviorGraphNode*				m_cachedHorizontalAdditiveInputNode;

public:
	CBehaviorGraphLookAtUsingAnimationsNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Look at (anim based, inputs)"); }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();

protected:
	Bool AreAdditiveInputsActive( CBehaviorGraphInstance& instance ) const;
	void UpdateControlValue( CBehaviorGraphInstance& instance, Float timeDelta, Bool dontBlend = false ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphLookAtUsingAnimationsNode );
	PARENT_CLASS( CBehaviorGraphLookAtUsingAnimationsCommonBaseNode );
	PROPERTY( m_cachedVerticalAdditiveInputNode );
	PROPERTY( m_cachedHorizontalAdditiveInputNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Struct defining names for animation and its additive corresponding "look at" animations
//

struct SLookAtAnimationPairDefinition
{
	DECLARE_RTTI_STRUCT( SLookAtAnimationPairDefinition );

	CName m_animationName;
	CName m_additiveHorizontalAnimationName;
	CName m_additiveVerticalAnimationName;

	SLookAtAnimationPairDefinition()
		:	m_animationName( CName::NONE )
		,	m_additiveHorizontalAnimationName( CName::NONE )
		,	m_additiveVerticalAnimationName( CName::NONE )
	{}

	void CollectUsedAnimations( TDynArray< CName >& anims ) const;
};

BEGIN_CLASS_RTTI( SLookAtAnimationPairDefinition );
	PROPERTY_CUSTOM_EDIT_NAME( m_animationName, TXT("Animation name"), TXT("This animation will have special replacing \"look at\" animations"), TXT("BehaviorAnimSelection") )
	PROPERTY_CUSTOM_EDIT_NAME( m_additiveHorizontalAnimationName, TXT("Horizontal anim (additive)"), TXT("Horizontal additive \"look at\" animation"), TXT("BehaviorAnimSelection") );
	PROPERTY_CUSTOM_EDIT_NAME( m_additiveVerticalAnimationName, TXT("Vertical anim (additive)"), TXT("Vertical additive \"look at\" animation"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Runtime instance of pair - contains animations found for this particular entity
//

struct SLookAtAnimationPairInstance
{
	DECLARE_RTTI_STRUCT( SLookAtAnimationPairInstance );

	const CSkeletalAnimationSetEntry* m_animation;
	const CSkeletalAnimationSetEntry* m_additiveHorizontalAnimation;
	const CSkeletalAnimationSetEntry* m_additiveVerticalAnimation;

	SLookAtAnimationPairInstance()
		:	m_animation( NULL )
		,	m_additiveHorizontalAnimation( NULL )
		,	m_additiveVerticalAnimation( NULL )
	{}

	void Setup( CBehaviorGraphInstance& instance, const SLookAtAnimationPairDefinition& definition );
};

BEGIN_CLASS_RTTI( SLookAtAnimationPairInstance );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Struct defining rules for choosing pair of additive "look at" animations
//

struct SLookAtAnimationPairInputBasedDefinition
{
	DECLARE_RTTI_STRUCT( SLookAtAnimationPairInputBasedDefinition );

	String m_inputName;
	CName m_additiveHorizontalAnimationName;
	CName m_additiveVerticalAnimationName;
	CBehaviorGraphValueNode* m_cachedInputNode;

	SLookAtAnimationPairInputBasedDefinition()
		:	m_inputName( TXT("Input") )
		,	m_additiveHorizontalAnimationName( CName::NONE )
		,	m_additiveVerticalAnimationName( CName::NONE )
		,	m_cachedInputNode( NULL )
	{}

	void CollectUsedAnimations( TDynArray< CName >& anims ) const;

	void CacheConnections( CBehaviorGraphNode* owner );
};

BEGIN_CLASS_RTTI( SLookAtAnimationPairInputBasedDefinition );
	PROPERTY_EDIT_NAME( m_inputName, TXT("Input name"), TXT("Input name for value ranging 0 to 1") )
	PROPERTY_CUSTOM_EDIT_NAME( m_additiveHorizontalAnimationName, TXT("Horizontal anim (additive)"), TXT("Horizontal additive \"look at\" animation"), TXT("BehaviorAnimSelection") );
	PROPERTY_CUSTOM_EDIT_NAME( m_additiveVerticalAnimationName, TXT("Vertical anim (additive)"), TXT("Vertical additive \"look at\" animation"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Runtime instance of pair - contains animations found for this particular entity
//

struct SLookAtAnimationPairInputBasedInstance
{
	DECLARE_RTTI_STRUCT( SLookAtAnimationPairInputBasedInstance );

	Float m_usageWeight;
	const CSkeletalAnimationSetEntry* m_additiveHorizontalAnimation;
	const CSkeletalAnimationSetEntry* m_additiveVerticalAnimation;

	SLookAtAnimationPairInputBasedInstance()
		:	m_usageWeight( 0.0f )
		,	m_additiveHorizontalAnimation( NULL )
		,	m_additiveVerticalAnimation( NULL )
	{}

	void Setup( CBehaviorGraphInstance& instance, const SLookAtAnimationPairInputBasedDefinition& definition );
};

BEGIN_CLASS_RTTI( SLookAtAnimationPairInputBasedInstance );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Uses embedded animations to be used for look at
//

class CBehaviorGraphLookAtUsingEmbeddedAnimationsNode : public CBehaviorGraphLookAtUsingAnimationsCommonBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLookAtUsingEmbeddedAnimationsNode, CBehaviorGraphLookAtUsingAnimationsCommonBaseNode, "Look ats", "Look at (embedded animation based)" );

protected:
	Bool																m_useHorizontalAnimations;
	Bool																m_useVerticalAnimations;
	SLookAtAnimationPairDefinition										m_defaultPair; // default pair of animation when no other pair can be found
	TDynArray< SLookAtAnimationPairInputBasedDefinition >				m_inputBasedPairs; // default pairs of animation based on input values
	TDynArray< SLookAtAnimationPairDefinition >							m_pairs; // pairs of animations that correspond to given animation

protected:
	TInstanceVar< Float >												i_timeDelta;
	TInstanceVar< SLookAtAnimationPairInstance >						i_defaultPair;
	TInstanceVar< TDynArray< SLookAtAnimationPairInputBasedInstance > >	i_inputBasedPairs;
	TInstanceVar< TDynArray< SLookAtAnimationPairInstance > >			i_pairs;
	TInstanceVar< SSimpleAnimationPlaybackSet >							i_horizontalPlaybacks;
	TInstanceVar< SSimpleAnimationPlaybackSet >							i_verticalPlaybacks;

public:
	CBehaviorGraphLookAtUsingEmbeddedAnimationsNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Look at (anim based, embedded)"); }
	virtual void OnRebuildSockets();
	virtual void OnPropertyPostChange( IProperty* property );
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void CollectUsedAnimations( TDynArray< CName >& anims ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

protected:
	RED_INLINE const SLookAtAnimationPairInstance* FindPairFor( CBehaviorGraphInstance& instance, const CSkeletalAnimationSetEntry* animation ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphLookAtUsingEmbeddedAnimationsNode );
	PARENT_CLASS( CBehaviorGraphLookAtUsingAnimationsCommonBaseNode );
	PROPERTY_EDIT( m_useHorizontalAnimations, TXT( "Use horizontal look at animations" ) );
	PROPERTY_EDIT( m_useVerticalAnimations, TXT( "Use vertical look at animations" ) );
	PROPERTY_EDIT_NAME( m_defaultPair, TXT("Default pair"), TXT("Default pair of animation when no other pair can be found") );
	PROPERTY_EDIT_NAME( m_inputBasedPairs, TXT("Input based pairs"), TXT("Input based pairs that override default pair") );
	PROPERTY_EDIT_NAME( m_pairs, TXT("Pairs"), TXT("Pairs of animations that correspond to given animation") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
