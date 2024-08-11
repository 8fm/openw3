/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "idEvent.h"
#include "idEventSenderDataStructs.h"


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEventSound : public CIdEvent
{	
	DECLARE_ENGINE_CLASS( CIdEventSound, CIdEvent, 0 );

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	SIDSoundEventParams		m_data;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate	( CIDTopicInstance* topicInstance );
};

BEGIN_CLASS_RTTI( CIdEventSound )
	PARENT_CLASS( CIdEvent )
	PROPERTY_INLINED( m_data, TXT("") )
	END_CLASS_RTTI()