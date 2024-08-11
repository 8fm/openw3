/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "idEvent.h"


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEventFact : public CIdEvent
{	
	DECLARE_ENGINE_CLASS( CIdEventFact, CIdEvent, 0 );

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	String	m_fact;
	Bool	m_enable;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate	( CIDTopicInstance* topicInstance );
};

BEGIN_CLASS_RTTI( CIdEventFact )
	PARENT_CLASS( CIdEvent )
	PROPERTY_INLINED( m_fact, TXT("") )
	PROPERTY_INLINED( m_enable, TXT("") )
END_CLASS_RTTI()