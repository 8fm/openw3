/**
* Copyright ©2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "entityTargetingAction.h"
#include "entityEventsNamesProvider.h"

class CActionBroadcastEvent : public IEntityTargetingAction, public IEntityEventsNamesProvider
{
	DECLARE_ENGINE_CLASS( CActionBroadcastEvent, IEntityTargetingAction, 0 );
private:
	CName m_eventToBrodecast;

	void PerformOnEntity( CEntity* parent ) override;
public:
	CActionBroadcastEvent()
		: IEntityTargetingAction( ETAM_Self )													{}
};

BEGIN_CLASS_RTTI( CActionBroadcastEvent );
	PARENT_CLASS( IEntityTargetingAction );
	PROPERTY_CUSTOM_EDIT( m_eventToBrodecast, TXT("Event send to router in this entity"), TXT("2daValueSelection") );
END_CLASS_RTTI();