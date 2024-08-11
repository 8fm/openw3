/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/behaviorGraphAnimationSlotNode.h"

class ActorActionPlayAnimation : public ActorAction, public ISlotAnimationListener
{
public:
	CName		m_animationSlot;		//!< Slot to play the animation on
	CName		m_animationName;		//!< Played animation
	Bool		m_animationInProgress;	//!< Is animation in progress
	Bool		m_continuePlaying;		//!<

public:
	ActorActionPlayAnimation( CActor* actor );
	~ActorActionPlayAnimation();

	//! Start animation
	Bool Start( const CName& slotName, const CName& animationName, Float blendIn, Float blendOut, Bool continuePlaying );

	//! Update, if false returned no further updates will be done
	virtual Bool Update( Float timeDelta );

	//! Stop animation
	virtual void Stop();

	//! Called when an actor is being pushed by another actor
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection ) {}

protected:
	//! Animation in slot has ended
	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status );
	virtual String GetListenerName() const { return TXT("ActorActionPlayAnimation"); }
};
