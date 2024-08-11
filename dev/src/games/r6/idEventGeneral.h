/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "idEvent.h"
#include "idEventSenderDataStructs.h"

struct SGeneralEventData;


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEventGeneral : public CIdEvent
{	
	DECLARE_ENGINE_CLASS( CIdEventGeneral, CIdEvent, 0 );

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	SGeneralEventData	m_data;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate			( CIDTopicInstance* topicInstance );

protected:
	void			RaiseGeneralEvent	( CEntity* entity, CName eventName );
};

BEGIN_CLASS_RTTI( CIdEventGeneral )
	PARENT_CLASS( CIdEvent )
	PROPERTY_INLINED( m_data, TXT("") )
END_CLASS_RTTI()
