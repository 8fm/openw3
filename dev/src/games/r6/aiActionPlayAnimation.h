/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiAction.h"


class CAIActionPlayAnimation : public CAIAction, public ISlotAnimationListener
{
	DECLARE_AI_ACTION_CLASS( CAIActionPlayAnimation, CAIAction )

protected:
	// const data
	CName							m_slotName;
	CName							m_animationName;
	Float							m_blendIn;
	Float							m_blendOut;
	Float							m_speed;		
	Float							m_offset;		
	Bool							m_looped;
	Bool							m_useMotionExtraction;		

	// runtime data
	THandle< CAnimatedComponent >	m_component;

public:
	CAIActionPlayAnimation();	
	
protected:
	// CAIAction interface
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual EAIActionStatus StartOn( CComponent* component )	override;
	virtual EAIActionStatus Stop( EAIActionStatus newStatus )	override;
	virtual EAIActionStatus Reset()								override;
	virtual Bool ShouldBeTicked() const							override { return false; }

protected:
	// ISlotAnimationListener interface
	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode* sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status ) override;
	virtual String GetListenerName() const { return GetClass()->GetName().AsString(); }
};

BEGIN_CLASS_RTTI( CAIActionPlayAnimation )
	PARENT_CLASS( CAIAction )
	PROPERTY_EDIT( m_slotName, TXT("Animation slot name.") )
	PROPERTY_EDIT( m_animationName, TXT("Name of the animation to be played.") )
	PROPERTY_EDIT_RANGE( m_blendIn, TXT("Blend in time in seconds."), 0.f, FLT_MAX )
	PROPERTY_EDIT_RANGE( m_blendOut, TXT("Blend out time in seconds."), 0.f, FLT_MAX )
	PROPERTY_EDIT( m_speed, TXT("Speed multiplier") )	
	PROPERTY_EDIT_RANGE( m_offset, TXT("Starting offset [0-1)"), 0.f, 1.f )		
	PROPERTY_EDIT( m_looped, TXT("") )
	PROPERTY_EDIT( m_useMotionExtraction, TXT("") )		
END_CLASS_RTTI()
