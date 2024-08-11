/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiAction.h"


class CAIActionInteraction : public CAIAction, public ISlotAnimationListener
{
	DECLARE_AI_ACTION_CLASS( CAIActionInteraction, CAIAction )

protected:
	// runtime data
	THandle< CMovingAgentComponent >	m_component;
	THandle< CR6InteractionComponent >	m_interaction;
	Bool								m_interruptionRequested;

public:
	CAIActionInteraction();	
	
protected:
	// CAIAction interface
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual EAIActionStatus StartOn( CComponent* component )	override;
	virtual EAIActionStatus Tick( Float timeDelta )				override;
	virtual EAIActionStatus RequestInterruption()				override;
	virtual EAIActionStatus Stop( EAIActionStatus newStatus )	override;
	virtual EAIActionStatus Reset()								override;
	virtual Bool ShouldBeTicked() const							override { return true; }

protected:
	// ISlotAnimationListener interface
	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode* sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status ) override;
	virtual String GetListenerName() const { return GetClass()->GetName().AsString(); }


	/// @return NULL if no interaction is found.
	CR6InteractionComponent* GetCurrentInteractionComponent() const;

private:
	RED_INLINE Bool CheckInteractPosition( const Vector& pos1, const Vector& pos2, Float tol ) const;
	RED_INLINE Bool CheckInteractRotation( Float rot1, Float rot2, Float tol ) const; 
};

BEGIN_CLASS_RTTI( CAIActionInteraction )
	PARENT_CLASS( CAIAction )
END_CLASS_RTTI()
