/**
* Copyright ©2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityEventsNamesProvider.h"

class IPerformableAction;

struct SEntityActionsRouterEntry
{
	DECLARE_RTTI_STRUCT( SEntityActionsRouterEntry );	

	CName								m_eventName;
	TDynArray< IPerformableAction*	>	m_actionsToPerform;
};

BEGIN_CLASS_RTTI( SEntityActionsRouterEntry )			
	PROPERTY_CUSTOM_EDIT( m_eventName			, TXT( "" ), TXT("2daValueSelection") );
	PROPERTY_INLINED	( m_actionsToPerform	, TXT( "" ) );	
END_CLASS_RTTI();

class IEntityActionsRouter : public IEntityEventsNamesProvider
{

protected:
	TDynArray< SEntityActionsRouterEntry > m_events;

public:
	void RouteEvent( CName eventName, CEntity* parent );

	virtual ~IEntityActionsRouter(){} // to have vtable, to use dynamic casts
};