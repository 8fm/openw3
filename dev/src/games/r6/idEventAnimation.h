/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "idEvent.h"
#include "idEventSenderDataStructs.h"


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEventAnimation : public CIdEvent
{	
	DECLARE_ENGINE_CLASS( CIdEventAnimation, CIdEvent, 0 );

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	SAnimationEventData		m_data;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate	( CIDTopicInstance* topicInstance );
};

BEGIN_CLASS_RTTI( CIdEventAnimation )
	PARENT_CLASS( CIdEvent )
	PROPERTY_INLINED( m_data, TXT("") )
END_CLASS_RTTI()