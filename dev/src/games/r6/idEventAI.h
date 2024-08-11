/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "idEvent.h"

#include "idEventSenderDataStructs.h"

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEventAI : public CIdEvent
{	
	DECLARE_ENGINE_CLASS( CIdEventAI, CIdEvent, 0 );

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	SAIEventData	m_data;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate	( CIDTopicInstance* topicInstance );
};

BEGIN_CLASS_RTTI( CIdEventAI )
	PARENT_CLASS( CIdEvent )
	PROPERTY_INLINED( m_data, TXT("") )
END_CLASS_RTTI()