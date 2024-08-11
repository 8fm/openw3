/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "..\..\common\engine\behaviorGraphNotifier.h"

class ActorActionRaiseEvent	: public ActorAction
{
protected:
	Bool							m_inProgress;			//!< Is process in progress
	Float							m_timer;
	CName							m_notificationName;
	EBehaviorGraphNotificationType	m_notificationType;

public:
	ActorActionRaiseEvent( CActor* actor );
	~ActorActionRaiseEvent();

	//! Update, if false returned no further updates will be done
	virtual Bool Update( Float timeDelta );

	//! Stop
	virtual void Stop();

	//! Called when an actor is being pushed by another actor
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection ) {}

public:
	//! Start
	Bool Start( const CName& eventName, const CName& notificationName, EBehaviorGraphNotificationType notificationType, Float timeout, Bool isForce = false );
};
