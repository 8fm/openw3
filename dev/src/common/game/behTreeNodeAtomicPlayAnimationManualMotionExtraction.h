/*
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicPlayAnimation.h"
#include "../engine/behaviorGraphAnimationManualSlot.h"

class CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance;

class CBehTreeNodeAtomicPlayAnimationManualMotionExtractionDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicPlayAnimationManualMotionExtractionDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance, PlayAnimationManualMotionExtraction );

protected:
	CName	m_slotName;
	CName	m_animationName;
	Uint32	m_loopIterations;
	Bool	m_isTransitionAnimation;

public:
	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicPlayAnimationManualMotionExtractionDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
	PROPERTY_EDIT( m_slotName, TXT("Animation slot name") );
	PROPERTY_EDIT( m_animationName, TXT("Animation name") );
	PROPERTY_EDIT( m_loopIterations, TXT("Number of loop iterations") );
	PROPERTY_EDIT( m_isTransitionAnimation, TXT("Is start point of animation different from end point") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;

protected:
	CName								m_slotName;
	CName								m_animationName;
	Uint32								m_loopIterations;
	Bool								m_isTransitionAnimation;

	Float								m_animationTime;
	CBehaviorManualSlotInterface		m_slot;
	const CSkeletalAnimationSetEntry*	m_anim;
	Uint32								m_currLoopIteration;

	Vector								m_initPos;
	EulerAngles							m_initRot;
	AnimQsTransform						m_initTrans;

	Bool								m_lastTeleport;

public:

	typedef CBehTreeNodeAtomicPlayAnimationManualMotionExtractionDefinition Definition;

	CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	// Base interface
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
};