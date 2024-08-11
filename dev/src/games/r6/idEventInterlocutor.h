/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "idEvent.h"
#include "idEventSenderDataStructs.h"


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEventInterlocutor : public CIdEvent
{	
	DECLARE_ENGINE_CLASS( CIdEventInterlocutor, CIdEvent, 0 );
	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	SInterlocutorEventData	m_data;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate			( CIDTopicInstance* topicInstance );

protected:
	void			RaiseGeneralEvent	( CEntity* entity, CName eventName );
};

BEGIN_CLASS_RTTI( CIdEventInterlocutor )
	PARENT_CLASS( CIdEvent )
	PROPERTY_INLINED( m_data, TXT("") )
	END_CLASS_RTTI()